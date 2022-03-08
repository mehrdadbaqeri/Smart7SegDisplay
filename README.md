# Smart7SegDisplay
A flexible display system based on intelligent LED modules, each of them able to display one character. One display module, consisting of a LED matrix with 8x5 pixels and an Atmel AVR Atmega16 controlling it. An arbitrary number of such intelligent display modules (up to 127) should be able to work together to form a display with that size. The display modules should be connected to each other by a serial bus with as few wires as possible. One or more connected display(s) should be able to display a complete text by shifting characters to the left or right.

![image](https://user-images.githubusercontent.com/7360143/157333620-69328c6d-6d34-4e0f-88e1-cefc7154b840.png)


# Hardware interface
The hardware interface consists of an I2C bus with two additional control lines, L and R to detect left and right modules.

![image](https://user-images.githubusercontent.com/7360143/157334285-46be73b4-8279-4250-a4c6-efeda6c7cb85.png)

