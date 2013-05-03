/*
	Copyright (C) 2003-2013 by David White <davewx7@gmail.com>
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <algorithm>

#include "key.hpp"
#include "string_utils.hpp"

CKey::CKey() : is_enabled(true), require_key_release(false), num_keys(300)
{
#if !SDL_VERSION_ATLEAST(2, 0, 0)
	key_list = SDL_GetKeyState( &num_keys );
#endif
}

int CKey::operator[]( int code ) const
{
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
	return 0;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 0)
	key_list = SDL_GetKeyboardState(&num_keys);
#endif
	if(require_key_release) {
		if(std::count(key_list, key_list + num_keys, 0) == num_keys) {
			require_key_release = false;
		}

		return 0;
	}

	return (code == SDLK_ESCAPE || is_enabled) && int(key_list[code]);
}

void CKey::SetEnabled( bool enable )
{
	is_enabled = enable;
}

void CKey::RequireRelease()
{
	require_key_release = true;
}

void CKey::Write(std::string* output)
{
	char buf[128];
	for(int n = 0; n != num_keys; ++n) {
		if(key_list[n]) {
			sprintf(buf, "%d", n);
			if(output->empty() == false) {
				*output += ",";
			}
			*output += buf;
		}
	}
}

void CKey::Read(const std::string& input)
{
	key_list = custom_key_list;
	if(num_keys > sizeof(custom_key_list)) {
		num_keys = sizeof(custom_key_list);
	}

	memset(custom_key_list, 0, sizeof(custom_key_list));

	std::vector<std::string> keys = util::split(input);
	for(int n = 0; n != keys.size(); ++n) {
		const int nkey = atoi(keys[n].c_str());
		if(nkey >= 0 && nkey < num_keys) {
			key_list[nkey] = 1;
		}
	}
}
