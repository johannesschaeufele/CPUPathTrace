#!/bin/sh

find include/ src/ demo/ test/ fuzz/ benchmark/ -iname *.h -o -iname *.cpp | xargs clang-format -i
