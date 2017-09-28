Termika Fotilo
==============

Termika Fotilo is Esperanto for thermal camera. The project, published in
[Make: Magazin 3/2017 on page 16](https://www.heise.de/select/make/2017/3/1498421900241703), 
describes a very limited infrared camera based on the relatively cheap (around
EUR 40) Melexis MLX90621 infrared sensor with a resolution of 16x4 pixels.

Origin
------

This is in turn inspired by the [DIY-Thermocam of Max Ritter](http://www.diy-thermocam.net/)
which was presented on the Maker Faire 2016 in Berlin. Ritter's kit uses a Flir
Lepton 3 infrared sensor with a resolution of 160x120 pixels (with a Lepton 2.5
the resolution is 80x60). That sensor is a lot better, but also much more expensive.

Finally, this repository aims to port the Termika Fotilo sketchbook, written for
use with an Arduino Nano, over to the ESP8266 plattform. It may be used with so
called dev boards, like the NodeMCU, which already comes with a voltage regulator
for a powering over USB or a battery pack, but these files are provided with use
of a bare ESP-12-E or ESP-12-F in mind (ESP-7 or earlier ESP-12 revisions should
work, but are untested: check your pinout diagrams).

Motivation
----------

If you do have a Arduino Nano at hand, then please just go for the Make: Magazin
article, which has very detailed instructions how to set up and may be purchased
for just EUR 3.29 as a PDF, in case you do not own that issue of the magazine.

The reason I am using an ESP chip instead is simply because I have them
already lying around, they are dirt cheap (currently around USD 1.70) and even
more compact then an Arduino Nano. The downside is that when using a bare ESP we
do have to add a few components for getting some pins to the right level on boot,
for voltage stability and of course the whole WiFi part of the ESP goes completely
unused.

Here below is schema showing how to set this up with a bare, flashed ESP chip.
When using a NodeMCU, the schema from the Make: article can be applied,
obviously choosing the appropriate SPI- and I²C-pins of the NodeMCU.

![Termika Fotilo schema using an ESP8266](https://raw.githubusercontent.com/elrido/TermikaFotilo/master/Termika%20Fotilo%20ESP8266.png)


License
-------

I did not find any license information on the source code provided by Make:
Magazin, but assume that it is copyrighted by Felix Pfeifer and Florian
Schäffer, authors of the article in the magazine, and/or Maker Media GmbH.

Therefore this project is provided on the same conditions and I assume it is
required to legally own a copy of at least the article to make any use of it.

By no means is the publishing of this code in this repository meant to infringe
on this copyright or to imply that this code is under public domain or may be
used commercially. This is shared in good faith under the assumption that it is
used for fun or education by readers of the Make: Magazin.

Note that this repository is not hosted in Germany and the I do not live in that
jurisdiction either. According to my jurisdiction, if you take any offence to me
publishing this, you need to inform me (send an email to elrido@gmx.net) and I
will gladly comply and remove this content, but I am not liable for any
imaginary damage that this publication may be believed to have done or any other
made up lawyer costs (like that "Abmahnung" lunacy) that might apply in German
jurisdiction. If any actual damage was done and bad faith on my side can be
proven, then of course you may take me to court. But before that, lets be civil
and just solve the problem as outlined above, without feeding the trolls (lawyers).

