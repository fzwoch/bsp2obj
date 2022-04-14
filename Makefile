#
# bsp2obj
#
# Copyright (C) 2016 Florian Zwoch <fzwoch@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

CFLAGS := -Wall -Wno-incompatible-pointer-types -O2 `pkg-config --cflags glib-2.0`
LDFLAGS := `pkg-config --libs glib-2.0`

all: bsp2obj

bsp2obj: bsp2obj.o
	$(CC) $< $(LDFLAGS) -o $@

clean:
	rm -f bsp2obj *.o
