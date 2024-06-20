import matplotlib.pyplot as plt
import numpy as np
from scipy.io import wavfile
import scipy.signal as scisig

#start = 1024 * 20 + 1
samplerate, data = wavfile.read('sample.wav')
data = data[:, 0] * 0.1

nsamples = round(len(data) * float(16000) / samplerate)
data = scisig.resample(data, nsamples)
#step = len(data) // nsamples
#data = data[0::step]

#print(data[start:start+20])
# 2 bytes per sample, 2 channels -> 4 bytes per sample
# 512 bytes is 128 samples
data = data.astype(np.int16)

#for v in data:
#	print(v)
start_idx = 0
patch_size = 500000
while start_idx < len(data):
	with open('/dev/usbdrv_1', 'wb') as f:
		f.write(data[start_idx:start_idx + patch_size])
		start_idx += patch_size

