/*
  	AWSUpdater.h

	(c) 2024 F.Lesage

	This program is free software: you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
	or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
	more details.

	You should have received a copy of the GNU General Public License along
	with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef _AWSUpdater_h
#define	_AWSUpdater_h

class AWSUpdater {

	private:

		bool	debug_mode		= true;

		int			download_file( const char *, const char *, const char *, const char *, const char *, const char * );
		void		download_package( const char *, const char *, const char *, const char *, JsonArray );
		uint32_t	fs_free_space( void );

	public:

			AWSUpdater( void ) = default;
		bool check_for_new_files( const char *, const char *, const char *, const char * );
};

#endif
