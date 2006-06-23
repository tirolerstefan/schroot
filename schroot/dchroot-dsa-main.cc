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

#include "dchroot-dsa-main.h"
#include "dchroot-dsa-chroot-config.h"
#include "dchroot-dsa-session.h"

#include <cstdlib>
#include <iostream>
#include <locale>

#include <termios.h>
#include <unistd.h>

#include <boost/format.hpp>

#include <syslog.h>

using std::endl;
using boost::format;
using namespace dchroot_dsa;

main::main (schroot::options_base::ptr& options):
  schroot::main(options),
  use_dchroot_conf(false)
{
  this->program_name = "dchroot-dsa";
}

main::~main ()
{
}

void
main::action_config ()
{
  std::cout << "# "
	    << format(_("schroot configuration generated by %1% %2%"))
    % this->program_name % VERSION
	    << endl;
  // Help text at head of new config.
  std::cout << "# " << endl
	    << "# "
	    << _("To allow users access to the chroots, use the users or groups keys.") << endl;
  std::cout << "# "
	    << _("To allow passwordless root access, use the root-users or root-groups keys.") << endl;
  std::cout << "# "
	    << format(_("Remove '%1%' to use the new configuration."))
    % DCHROOT_CONF
	    << endl;
  std::cout << endl;
  this->config->print_chroot_config(this->chroots, std::cout);
}

void
main::action_list ()
{
  this->config->print_chroot_list_simple(std::cout);
}

void
main::compat_check ()
{
  if (this->options->verbose)
    {
      sbuild::log_warning()
	<< _("Running schroot in dchroot compatibility mode")
	<< endl;
      sbuild::log_info()
	<< _("Run 'schroot' for full capabilities")
	<< endl;
    }
}

void
main::load_config ()
{
  this->use_dchroot_conf = false;

  struct stat statbuf;
  if (stat(DCHROOT_CONF, &statbuf) == 0 && !S_ISDIR(statbuf.st_mode))
    {
      this->use_dchroot_conf = true;

      if (this->options->verbose)
	{
	  sbuild::log_warning()
	    << format(_("Using %1% configuration file: "))
	    % this->program_name
	    << DCHROOT_CONF
	    << endl;
	  sbuild::log_info()
	    << format(_("Run '%1%'"))
	    % "dchroot --config >> " SCHROOT_CONF
	    << endl;
	  sbuild::log_info()
	    << _("to migrate to a schroot configuration.")
	    << endl;
	  sbuild::log_info()
	    << format(_("Edit '%1%' to add appropriate group access."))
	    % SCHROOT_CONF
	    << endl;
	  sbuild::log_info()
	    << format(_("Remove '%1%' to use the new configuration."))
	    % DCHROOT_CONF
	    << endl;
	}
    }

  if (this->use_dchroot_conf)
    {
      this->config =
	sbuild::chroot_config::ptr(new dchroot_dsa::chroot_config);
      if (this->options->load_chroots == true)
	this->config->add(DCHROOT_CONF, false);
    }
  else
    {
      schroot::main::load_config();
    }
}

void
main::create_session(sbuild::session::operation sess_op)
{
  sbuild::log_debug(sbuild::DEBUG_INFO)
    << "Creating dchroot-dsa session" << endl;

  // Using dchroot.conf implies using dchroot_session_base, which does
  // not require user or group access.
  this->session = sbuild::session::ptr
    (new dchroot_dsa::session("schroot",
			      this->config,
			      sess_op,
			      this->chroots,
			      this->use_dchroot_conf));
}
