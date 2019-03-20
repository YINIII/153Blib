import serial
from pynput.keyboard import Key, Controller
import time
import os

if __name__ == '__main__':
    uart = serial.Serial('/dev/cu.usbserial-AI065VNI', 115200)
    uart.reset_input_buffer()

    keyboard = Controller()

    while True:
        if uart.in_waiting > 0:
            command = uart.read()
            if command == b'b':
                with keyboard.pressed(Key.cmd):
                    keyboard.press('t')
                    time.sleep(0.1)
                    keyboard.release('t')
                keyboard.type('emacs')
                keyboard.press(Key.enter)
                keyboard.release(Key.enter)
                keyboard.press(Key.esc)
                keyboard.release(Key.esc)
                keyboard.press('x')
                keyboard.release('x')
                keyboard.type('tetris')
                keyboard.press(Key.enter)
                keyboard.release(Key.enter)
            if command == b'u':
                keyboard.press(Key.up)
                time.sleep(0.1)
                keyboard.release(Key.up)
            elif command == b'd':
                keyboard.press(Key.down)
                time.sleep(0.1)
                keyboard.release(Key.down)
            elif command == b'l':
                keyboard.press(Key.left)
                time.sleep(0.1)
                keyboard.release(Key.left)
            elif command == b'r':
                keyboard.press(Key.right)
                time.sleep(0.1)
                keyboard.release(Key.right)
            else:


    uart.close()
