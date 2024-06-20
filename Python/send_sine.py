import matplotlib.pyplot as plt
import numpy as np
from scipy.io import wavfile
import scipy.signal as scisig

#start = 1024 * 20 + 1
samplerate, data = wavfile.read('sine.wav')
print(samplerate, len(data))
data = data.astype(np.int16) * 3

for v in data[:100]:
	print(v)
with open('/dev/usbdrv_1', 'wb') as f:
	f.write(data)

