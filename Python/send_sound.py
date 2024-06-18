import matplotlib.pyplot as plt
import numpy as np
#from scipy.io import wavfile

#start = 1024 * 20 + 1
#samplerate, data = wavfile.read('sample.wav')
#print(data[start:start+20])
# 2 bytes per sample, 2 channels -> 4 bytes per sample
# 512 bytes is 128 samples
'''
with open('/dev/usbdrv_1', 'wb') as f:
	f.write(data[start:start+128])
start += 128
with open('/dev/usbdrv_1', 'wb') as f:
	f.write(data[start:start+128])
start += 128
with open('/dev/usbdrv_1', 'wb') as f:
	f.write(data[start:start+128])
'''
signal = np.sin(np.linspace(0, 3.14159*1000, 150000)) * 10000
signal = signal.astype(np.int16)
plt.plot(signal)
plt.show()

with open('/dev/usbdrv_1', 'wb') as f:
	f.write(signal)

