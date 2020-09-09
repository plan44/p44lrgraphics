
p44lrgraphics
=============

*[[if you want to support p44lrgraphics development, consider to sponsor plan44]](https://github.com/sponsors/plan44)* 

*p44lrgraphics* is a set of free (opensource, GPLv3) C++ classes and functions building a graphics library specially targeted at **low resolution displays**.

A *low resolution display* is usually a LED matrix of a few 100 to 1000 LEDs, meaning a *total pixel count in the 1000s* range.

Even the smallest TFT displays start in the 100'000 pixel range, and any computer screen nowadays has millions of pixels. So, clearly, *p44lrgraphics* is **not** meant a generic graphics library (if you are looking for a small one suitable for embedded devices, I can recommend [LittlevGL](https://littlevgl.com)), which is also supported via [*p44utils*](https://github.com/plan44/p44utils).

*p44lrgraphics*'s view structure is similar to other framework's. However the implementation is mostly done following the needs of driving LED chains and matrices, in particular those of the WS281x variety.

One main design decision that only makes sense for a modest number of pixels but quickly changing imagery (light effects) is that *p44lrgraphics* is not focused on drawing things into image buffers and then flattening those down the view/layer hierarchy to a full screen image, displayed at a fixed frame rate.

Instead, *p44lrgraphics* mostly does exactly the opposite: each of the (relatively few) pixels asks from bottom **up** the view hierarchy for its current color, and the views then calculate the pixels *on demand*. This simplifies the code in many cases, and because of the relatively few pixels involved, it does not need a lot of performance. And it allows asynchronous updating timed to other events than those of a fixed frame rate.

*p44lrgraphics* needs some classes and functions from the [*p44utils*](https://github.com/plan44/p44utils) library.

Projects using p44lrgraphics (or cointaining roots that led to it) include 
the [pixelboard](https://github.com/plan44/pixelboard-hardware), the [ETH digital platform](https://plan44.ch/custom/custom.php#leth), and the "chatty wifi" installation I brought to the 35c3 (which is in the [*hermel* branch of *lethd*](https://github.com/plan44/lethd/tree/hermeld))


Usage
-----
*p44lrgraphics* sources meant to be included as .cpp and .hpp files into a project (usually as a git submodule) and compiled together with the project's other sources.

A configuration header file *p44lrg_config.hpp* needs to be present in the project, and allows customizing some aspects of *p44lrgraphics*.

To get started, just copy the *p44lrg_config_TEMPLATE.hpp* to a location in your include path and name it *p44lrg_config.hpp*.

License
-------

p44lrgraphics is licensed under the GPLv3 License (see COPYING).

If that's a problem for your particular application, I am open to provide a commercial license, please contact me at [luz@plan44.ch](mailto:luz@plan44.ch).

Features
--------

Details tbd. but quick list for now:

- views with 90 degree rotation, mirroring
- scrolling with subpixel resolution (antialiasing)
- png image views
- view stacks
- view animations
- Support for [Microsofts *expressive pixels* IoT animation format](https://github.com/microsoft/ExpressivePixels/wiki/Animation-Format) with the "epx" view
- on the fly reconfiguration via JSON API/config files

(c) 2013-2020 by Lukas Zeller / [plan44.ch](https://www.plan44.ch/opensource.php)
