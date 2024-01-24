#pragma once

#include <string>

#include <string.h>

namespace Argv
{

#include <getopt.h>

using GNUOption = struct option;

static constexpr auto GNUOptionDone = GNUOption{nullptr, 0, nullptr, 0};

static constexpr auto make_Option(char const* long_flag, int short_flag)
{
	return GNUOption{long_flag, no_argument, nullptr, short_flag};
}

static constexpr auto make_Param(char const* long_flag, int short_flag)
{
	return GNUOption{long_flag, required_argument, nullptr, short_flag};
}

class IncomingArgv
{
private: // members
	GNUOption const* m_opts;
	int m_argc;
	char** m_argv;
	std::string m_flags;
	int m_optind;

public: // interface
	IncomingArgv(GNUOption const* opts, int argc, char** argv)
		: m_opts{opts}
		, m_argc{argc}
		, m_argv{argv}
		, m_flags{"+"}
		, m_optind{}
	{
		// Build flags spec
		for (auto opt = m_opts; opt->val != 0; opt++) {

			// Add short option
			if (::isalpha(opt->val)) {
				m_flags.push_back((char)(opt->val));

				// Add argument flag
				if (opt->has_arg != no_argument) {
					m_flags.push_back(':');
				}
			}
		}
	}

	auto get_next()
	{
		// Continune parsing at saved option index
		auto optind_save = optind;
		optind = m_optind;

		auto c = getopt_long(m_argc, m_argv, m_flags.c_str(), m_opts, nullptr);

		// Restore optind
		m_optind = optind;
		optind = optind_save;

		return std::make_pair(c,
			((c < 0) || (optarg == nullptr))
				? std::string{}
				: std::string{optarg}
		);
	}

	auto get_rest()
	{
		return std::make_pair(m_argc - m_optind, m_argv + m_optind);
	}
};

} // namespace Argv