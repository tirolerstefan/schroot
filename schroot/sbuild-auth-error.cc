/* Copyright © 2005-2006  Roger Leigh <rleigh@debian.org>
 *
 * schroot is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * schroot is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *********************************************************************/

#include <config.h>

#include "sbuild-i18n.h"
#include "sbuild-auth-error.h"

#include <boost/format.hpp>

using namespace sbuild;
using boost::format;

namespace
{

  typedef std::pair<auth_error::type,const char *> emap;

  /**
   * This is a list of the supported error codes.  It's used to
   * construct the real error codes map.
   */
  emap init_errors[] =
    {
      emap(auth_error::NONE,            N_("No error")),
      emap(auth_error::HOSTNAME,        N_("Failed to get hostname")),
      emap(auth_error::USER,            N_("User not found")),
      emap(auth_error::AUTHENTICATION,  N_("Access not authorised")),
      emap(auth_error::AUTHORISATION,   N_("Authentication failed")),
      emap(auth_error::PAM_DOUBLE_INIT, N_("PAM is already initialised")),
      emap(auth_error::PAM,             N_("PAM error"))
    };

}

std::map<auth_error::type,const char *>
auth_error::error_strings
(init_errors,
 init_errors + (sizeof(init_errors) / sizeof(init_errors[0])));

const char *
auth_error::get_error (type error_type)
{
  std::map<type,const char *>::const_iterator pos =
    error_strings.find(error_type);

  if (pos != error_strings.end())
    return gettext(pos->second);

  return _("unknown error");
}

std::string
auth_error::format_error (std::string const& detail,
			  type               error_type)
{
  if (detail.length() > 0)
    {
      format fmt((error_type == NONE
		 ? "%2%"
		 : "%1%: %2%"));
      fmt % get_error(error_type) % detail;
      return fmt.str();
    }
  else
    return get_error(error_type);
}

std::string
auth_error::format_error (std::string const& detail,
			  type               error_type,
			  int                error_number)
{
  return format_error(detail, error_type, std::string(strerror(error_number)));
}

std::string
auth_error::format_error (std::string const& detail,
			  type               error_type,
			  std::string const& error_string)
{
  if (detail.length() > 0)
    {
      format fmt((error_type == NONE
		 ? "%1%: %3%"
		 : "%1%: %2%: %3%"));
      fmt % detail
	% get_error(error_type)
	% error_string;
      return fmt.str();
    }
  else
    {
      format fmt((error_type == NONE
		  ? "%2%"
		  : "%1%: %2%"));
      fmt % get_error(error_type)
	% error_string;
      return fmt.str();
    }
}
