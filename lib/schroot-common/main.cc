/* Copyright © 2005-2013  Roger Leigh <rleigh@debian.org>
 *
 * schroot is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * schroot is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 *********************************************************************/

#include <config.h>

#include "main.h"

#include <sbuild/config.h>
#ifdef SBUILD_FEATURE_PAM
#include <sbuild/auth/pam.h>
#include <sbuild/auth/pam-conv.h>
#include <sbuild/auth/pam-conv-tty.h>
#endif // SBUILD_FEATURE_PAM
#include <sbuild/keyfile-writer.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <locale>

#include <termios.h>
#include <unistd.h>

#include <boost/format.hpp>

using std::endl;
using boost::format;
using sbuild::_;
using sbuild::N_;
using namespace schroot_common;

template<>
sbuild::error<main::error_code>::map_type
sbuild::error<main::error_code>::error_strings =
  {
    // TRANSLATORS: %4% = file
    {main::CHROOT_FILE,       N_("No chroots are defined in ‘%4%’")},
    // TRANSLATORS: %4% = file
    // TRANSLATORS: %5% = file
    {main::CHROOT_FILE2,      N_("No chroots are defined in ‘%4%’ or ‘%5%’")},
    // TRANSLATORS: %1% = file
    {main::CHROOT_NOTDEFINED, N_("The specified chroots are not defined in ‘%1%’")},
    {main::SESSION_INVALID,   N_("%1%: Invalid session name")}
  };

main::main (std::string const& program_name,
            std::string const& program_usage,
            options::ptr&      options,
            bool               use_syslog):
  bin_common::main(program_name, program_usage,
                   std::static_pointer_cast<bin_common::options>(options),
                   use_syslog),
  options(options)
{
}

main::~main ()
{
}

void
main::action_version (std::ostream& stream)
{
  bin_common::main::action_version(stream);

  format feature("  %1$-12s %2%\n");

  stream << '\n'
         << _("Available chroot types:") << '\n';
#ifdef SBUILD_FEATURE_BLOCKDEV
  stream << feature % "BLOCKDEV" % _("Support for ‘block-device’ chroots");
#endif
#ifdef SBUILD_FEATURE_BTRFSSNAP
  stream << feature % "BTRFSSNAP" % _("Support for ‘btrfs-snapshot’ chroots");
#endif
  stream << feature % "CUSTOM" % _("Support for ‘custom’ chroots");
  stream << feature % "DIRECTORY" % _("Support for ‘directory’ chroots");
  stream << feature % "FILE" % _("Support for ‘file’ chroots");
#ifdef SBUILD_FEATURE_LOOPBACK
  stream << feature % "LOOPBACK" % _("Support for ‘loopback’ chroots");
#endif
#ifdef SBUILD_FEATURE_LVMSNAP
  stream << feature % "LVMSNAP" % _("Support for ‘lvm-snapshot’ chroots");
#endif
  stream << feature % "PLAIN" % _("Support for ‘plain’ chroots");
  stream << std::flush;
}

void
main::action_info ()
{
  for(auto chroot_name = this->chroot_names.begin();
      chroot_name != this->chroot_names.end();
      ++chroot_name)
    {
      // This should never fail, so no error handling here--we already
      // validated everything when we got the chroot map.
      sbuild::chroot::config::chroot_map::const_iterator c = this->chroots.find(*chroot_name);
      assert(c->second);

      std::cout << c->second;
      if (chroot_name + 1 != this->chroot_names.end())
        std::cout << '\n';
    }

  std::cout << std::flush;
}

void
main::action_location ()
{
  for(const auto& chroot_name : this->chroot_names)
    {
      // This should never fail, so no error handling here--we already
      // validated everything when we got the chroot map.
      const auto chroot = this->chroots.find(chroot_name);
      assert(chroot->second);

      std::cout << chroot->second->get_path() << '\n';
    }

  std::cout << std::flush;
}

void
main::action_config ()
{
  std::cout << "# "
    // TRANSLATORS: %1% = program name
    // TRANSLATORS: %2% = program version
    // TRANSLATORS: %3% = current date
            << format(_("schroot configuration generated by %1% %2% on %3%"))
    % this->program_name % VERSION % sbuild::date(time(0))
            << endl;
  std::cout << endl;

  sbuild::keyfile info;

  for(const auto& chroot_name : this->chroot_names)
    {
      // This should never fail, so no error handling here--we already
      // validated everything when we got the chroot map.
      const auto chroot = this->chroots.find(chroot_name);
      assert(chroot->second);

      // Generated chroots (e.g. source chroots) are not printed.
      if (chroot->second->get_original())
        info << chroot->second;
    }

  std::cout << sbuild::keyfile_writer(info) << std::flush;
}

void
main::get_chroot_options ()
{

  if (this->options->all_chroots == true ||
      this->options->all_sessions == true ||
      this->options->all_source_chroots == true)
    {
      if (this->options->all_chroots)
        {
          sbuild::string_list chroots;
          if (this->options->action == options::ACTION_LIST &&
              !this->options->exclude_aliases)
            chroots = this->config->get_alias_list("chroot");
          else
            chroots = this->config->get_chroot_list("chroot");
          this->chroot_names.insert(this->chroot_names.end(), chroots.begin(), chroots.end());
        }
      if (this->options->all_sessions)
        {
          sbuild::string_list sessions;
          if (this->options->action == options::ACTION_LIST &&
              !this->options->exclude_aliases)
            sessions = this->config->get_alias_list("session");
          else
            sessions = this->config->get_chroot_list("session");
          this->chroot_names.insert(this->chroot_names.end(), sessions.begin(), sessions.end());
        }
      if (this->options->all_source_chroots)
        {
          sbuild::string_list sources;
          if (this->options->action == options::ACTION_LIST &&
              !this->options->exclude_aliases)
            sources = this->config->get_alias_list("source");
          else
            sources = this->config->get_chroot_list("source");
          this->chroot_names.insert(this->chroot_names.end(), sources.begin(), sources.end());
        }

      // Validate and normalise
      this->chroots = this->config->validate_chroots("", this->chroot_names);
    }
  else
    {
      // Search in the appropriate namespace.
      std::string chroot_namespace("chroot");
      if (this->options->action == options::ACTION_SESSION_RECOVER ||
          this->options->action == options::ACTION_SESSION_RUN ||
          this->options->action == options::ACTION_SESSION_END)
        chroot_namespace = "session";

      // Validate and normalise
      this->chroot_names = this->options->chroots;
      this->chroots = this->config->validate_chroots
        (chroot_namespace, this->chroot_names);
    }
}

void
main::load_config ()
{
  this->config = sbuild::chroot::config::ptr(new sbuild::chroot::config);
  /* The normal chroot list is used when starting a session or running
     any chroot type or session, or displaying chroot information. */
  if (this->options->load_chroots == true)
    {
      this->config->add("chroot", SCHROOT_CONF);
      this->config->add("chroot", SCHROOT_CONF_CHROOT_D);
    }
  /* The session chroot list is used when running or ending an
     existing session, or displaying chroot information. */
  if (this->options->load_sessions == true)
    this->config->add("session", SCHROOT_SESSION_DIR);
}

int
main::run_impl ()
{
  if (this->options->action == options::ACTION_HELP)
    {
      action_help(std::cout);
      return EXIT_SUCCESS;
    }

  if (this->options->action == options::ACTION_VERSION)
    {
      action_version(std::cout);
      return EXIT_SUCCESS;
    }

  /* Initialise chroot configuration. */
  load_config();

  if (this->options->load_chroots &&
      this->config->get_chroots("chroot").empty() &&
      this->options->quiet == false)
    log_exception_warning(error(CHROOT_FILE2, SCHROOT_CONF, SCHROOT_CONF_CHROOT_D));

  /* Get list of chroots to use */
  get_chroot_options();
  if (this->chroots.empty())
    {
      if (!(this->options->all_chroots == true ||
            this->options->all_sessions == true ||
            this->options->all_source_chroots == true))
        throw error(SCHROOT_CONF, CHROOT_NOTDEFINED);
      else
        {
          // If one of the --all options was used, then don't treat
          // the lack of chroots as an error.  TODO: Also check if any
          // additional chroots were specified with -c; this needs
          // changes in get_chroot_options.
          if (this->options->verbose)
            log_exception_warning(error((this->options->all_chroots == true) ?
                                        SCHROOT_CONF : SCHROOT_SESSION_DIR,
                                        CHROOT_NOTDEFINED));
          return EXIT_SUCCESS;
        }
    }
  this->chroot_objects.clear();
  for(const auto& chroot_name : this->chroot_names)
    {
      // This should never fail, so no error handling here--we already
      // validated everything when we got the chroot map.
      const auto chroot = this->chroots.find(chroot_name);
      assert(chroot->second);
      sbuild::session::chroot_list::value_type e;
      e.alias = chroot->first;
      e.chroot = chroot->second;
      chroot_objects.push_back(e);
    }

  /* Print chroot list. */
  if (this->options->action == options::ACTION_LIST)
    {
      action_list();
      return EXIT_SUCCESS;
    }

  if (this->config->find_alias("session", this->options->session_name))
    throw error(this->options->session_name, SESSION_INVALID);

  /* Print chroot information for specified chroots. */
  if (this->options->action == options::ACTION_INFO)
    {
      action_info();
      return EXIT_SUCCESS;
    }
  if (this->options->action == options::ACTION_LOCATION)
    {
      action_location();
      return EXIT_SUCCESS;
    }
  if (this->options->action == options::ACTION_CONFIG)
    {
      action_config();
      return EXIT_SUCCESS;
    }

  /* Create a session. */
  sbuild::session::operation sess_op(sbuild::session::OPERATION_AUTOMATIC);
  if (this->options->action == options::ACTION_SESSION_BEGIN)
    sess_op = sbuild::session::OPERATION_BEGIN;
  else if (this->options->action == options::ACTION_SESSION_RECOVER)
    sess_op = sbuild::session::OPERATION_RECOVER;
  else if (this->options->action == options::ACTION_SESSION_RUN)
    sess_op = sbuild::session::OPERATION_RUN;
  else if (this->options->action == options::ACTION_SESSION_END)
    sess_op = sbuild::session::OPERATION_END;

  try
    {
      create_session(sess_op);
      add_session_auth();

      if (!this->options->command.empty())
        this->session->get_auth()->set_command(this->options->command);
      if (!this->options->directory.empty())
        this->session->get_auth()->set_wd(this->options->directory);
      if (!this->options->shell.empty())
        this->session->set_shell_override(this->options->shell);
      this->session->set_preserve_environment(this->options->preserve);
      this->session->set_session_id(this->options->session_name);
      this->session->set_force(this->options->session_force);
      if (this->options->quiet)
        this->session->set_verbosity("quiet");
      else if (this->options->verbose)
        this->session->set_verbosity("verbose");
      this->session->set_user_options(this->options->useroptions_map);

      /* Run session. */
      this->session->run();
    }
  catch (std::runtime_error const& e)
    {
      if (!this->options->quiet)
        sbuild::log_exception_error(e);
    }

  if (this->session)
    return this->session->get_child_status();
  else
    return EXIT_FAILURE;
}

void
main::add_session_auth ()
{
  // Add PAM authentication handler.  If PAM isn't available, just
  // continue to use the default handler

#ifdef SBUILD_FEATURE_PAM
  sbuild::auth::auth::ptr auth = sbuild::auth::pam::create("schroot");

  sbuild::auth::pam_conv::auth_ptr pam_auth =
    std::dynamic_pointer_cast<sbuild::auth::pam>(auth);

  sbuild::auth::pam_conv::ptr conv = sbuild::auth::pam_conv_tty::create(pam_auth);


  /* Set up authentication timeouts. */
  time_t curtime = 0;
  time(&curtime);
  conv->set_warning_timeout(curtime + 15);
  conv->set_fatal_timeout(curtime + 20);
  pam_auth->set_conv(conv);

  this->session->set_auth(auth);
#endif // SBUILD_FEATURE_PAM
}
