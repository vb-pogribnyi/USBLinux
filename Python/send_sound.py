'''
import numpy as np
from pydub import AudioSegment

sound = AudioSegment.from_mp3('Hotlanta - Track Tribe.mp3')

raw_data = sound._data
buff = np.frombuffer(raw_data, dtype='int16')

start = 1024 * 20
print(len(buff), buff[start:start+512*8])
print(ord('s'))

exit()
with open('/dev/usbdrv_1', 'wb') as f:
	f.write([ord('s')] + buff[start:start+8])
'''

import numpy as np
from scipy.io import wavfile

start = 1024 * 20 + 1
samplerate, data = wavfile.read('sample.wav')
print(data[start:start+20])
# 2 bytes per sample, 2 channels -> 4 bytes per sample
# 512 bytes is 128 samples
with open('/dev/usbdrv_1', 'wb') as f:
	f.write(data[start:start+128])
start += 128
with open('/dev/usbdrv_1', 'wb') as f:
	f.write(data[start:start+128])
start += 128
with open('/dev/usbdrv_1', 'wb') as f:
	f.write(data[start:start+128])

