SLIC - simple lossless imaging codec
------------------------------------
Copyright (c) 2022 BitBank Software, Inc.<br>
Written by Larry Bank<br>
bitbank@pobox.com<br>

What is it?
-----------
An 'embedded-friendly' (aka Arduino) image compression format/codec that supports most common pixel types, is fast to compress, fast to decompress and easily adds useful compression to many projects that would otherwise be unable to use popular formats like JPEG/PNG/TIFF/WebP/etc<br>

What isn't it?
--------------
A perfect image codec that solves every problem and fills every need. As the name implies, it's a simple yet effective format that opens a few new doors.<br>

Why did you write it?
---------------------
I think there is a need to fill a gap in the current image compression standards - a fast+simple lossless codec that can run well on resource constrained (e.g. embedded) systems. Written in C99, it doesn't require a file system nor the C-runtime library to function, so it's perfectly suited to run on small microcontrollers with limited memory. It also directly supports RGB565 pixels, which seems to be pretty rare in the image file format world. It's extremely fast (of course) and has a good worst-case behavior. The compression scheme uses run-length, palette/cache and difference coding. It works well on many types of images, except for photos with many unique colors. I specifically tailored the compression to work best on screenshots, charts, and 'computerish' images.

Why does it look so similar to QOI?
-----------------------------------
I started working with the QOI format and it gave me some good ideas as a starting point, but the stated goals and limitations of the format were preventing me from supporting that project.

What's special about it?
------------------------
As with all of my recent projects, my aim is to make it fast, simple and easy to integrate. What's special is that it works so effectively on a wide range of images, doesn't require much RAM nor a file system and has a simple API accessible as C or C++. It's extremely easy to integrate into any project and doesn't take up much code space nor RAM.<br>

Click the image below to see a Youtube video of it running on a 8Mhz ATMega328 (8-bit MCU w/2K RAM, 32K FLASH) decoding images to a 320x240 SPI LCD.
[![SLIC running on ATMega328](https://img.youtube.com/vi/8YeGYrjqi5Y/0.jpg)](https://www.youtube.com/watch?v=8YeGYrjqi5Y)

<br>
Feature summary:<br>
----------------<br>
- Runs on any CPU/MCU with at least 1K of free RAM<br>
- No external dependencies (including malloc/free)<br>
- Allows memory to memory encode and decode<br>
- Can work with files through callback functions you provide<br>
- Extremely fast yet extremely effective at compressing many types of images<br>
- Encode and decode an image by as few or as many pixels at a time as you like<br>
- Supported Pixel types: 8-bit grayscale, 8-bit palette, 24/32-bit truecolor, and RGB565<br>
- Arduino-style C++ library class with simple API<br>
- Can by built as a straight C project as well<br>
<br>

FAQ:<br>
----<br>

Q0: Does it run on the Arduino Uno?<br>
A0: Yes! You can both encode and decode images on the Uno. 8-bit Palette images are more difficult because you need a 768-byte palette.<br>
<br>
Q1: How does it perform compared to PNG and GIF?<br>
A1: It's faster than both, but usually underperforms both in terms of compression. SLIC can sometimes compress better than PNG for 24 or 32-bpp images.<br>
<br>
Q2: What type of images does SLIC compress well?<br>
A2: SLIC does well with smooth gradients and areas of common colors.<br>
<br>
Q3: What type of images does SLIC compress poorly?<br>
A3: Images with shot noise or lots of unique colors (e.g. photos of people/places) don't compress well.<br>
<br>
Q4: What's the easiest way to start using SLIC images in my project?<br>
A4: Peruse the Wiki for info about the API, create some ".slc" files with the command line tool in the linux directory and start your code from one of the examples.<br>
<br>
Q5: How can I easily create SLIC-compressed images from existing image files?<br>
A5: The command line tool in the linux directory should build correctly on MacOS+Windows as well. Use it to convert Windows BMP files into .slc files.<br>
