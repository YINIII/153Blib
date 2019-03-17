import serial
import wave

if __name__ == '__main__':
    uart = serial.Serial('/dev/cu.usbserial-AI065VNI', 115200)
    uart.reset_input_buffer()

    print('Voice recognition ready')
    while True:
        if uart.read(3) == b'bgn': # start signal
            print('Start recording...')
            f = wave.open('voice.wav', 'wb')
            f.setparams((2, 2, 48000, 0, 'NONE', 'NONE'))
            read_buff = []
            byte_count = 0
            while b''.join(read_buff) != b'stp': # stop signal
                if uart.in_waiting > 0:
                    read_buff.append(uart.read())
                    byte_count += 1
                    if len(read_buff) > 3:
                        byte = read_buff.pop(0)
                        f.writeframes(byte)
            print('Recording complete')
            print(byte_count, 'bytes received')
            f.close()

            # Voice Recognition code starts here
            # data stored in voice.wav

    uart.close()
