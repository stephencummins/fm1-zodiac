import wave, numpy as np
for s in "aries taurus gemini cancer leo virgo libra scorpio sagittarius capricorn aquarius pisces".split():
    w = wave.open('build/sound/%s.wav' % s); sr = w.getframerate()
    a = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').reshape(-1, 2) / 32768.0
    m = a.mean(1); r = np.array([np.sqrt((m[i*sr:(i+1)*sr]**2).mean()) for i in range(len(m)//sr)])
    n = min(len(m), sr*60); f = np.abs(np.fft.rfft(m[:n]*np.hanning(n))); fr = np.fft.rfftfreq(n, 1/sr)
    b = lambda lo, hi: 10*np.log10((f[(fr >= lo) & (fr < hi)]**2).sum() + 1e-9)
    t = b(0, 22050)
    print('%-12s peak %.2f rms %.3f 1s min/max %.3f/%.3f clip %5d | <120 %3.0f  120-500 %3.0f  500-2k %3.0f  2k-8k %3.0f  >8k %3.0f dB | L/R %.2f' % (
        s, abs(a).max(), r.mean(), r.min(), r.max(), (abs(a) > 0.98).sum(), b(0, 120)-t, b(120, 500)-t, b(500, 2000)-t, b(2000, 8000)-t, b(8000, 22050)-t,
        np.sqrt((a[:, 0]**2).mean())/np.sqrt((a[:, 1]**2).mean())))
