/*
Copyright(c) 2017 Cedric Jimenez

This file is part of Nano-IP.

Nano-IP is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Nano-IP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Nano-IP.  If not, see <http://www.gnu.org/licenses/>.
*/


/** \brief Pointer to static C++ constructor */
typedef void (*pfStaticCtor)(void);


/** \brief Call constructors of all statics objects */
void cpp_init(void)
{
    extern char _CTORS_START[];
    extern char _CTORS_END[];

    pfStaticCtor* static_ctor = (pfStaticCtor*)_CTORS_START;
    while( static_ctor != (pfStaticCtor*)_CTORS_END )
    {
        (*static_ctor)();
        static_ctor++;
    }
}

