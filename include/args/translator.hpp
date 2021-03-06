// Copyright (c) 2018 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <args/api.hpp>
#include <string>
#include <string_view>

namespace args {
	enum class lng : int {
		usage,
		def_meta,
		positionals,
		optionals,
		help_description,
		unrecognized,
		needs_param,
		needs_no_param,
		needs_number,
		needed_number_exceeded,
		needed_enum_unknown,
		needed_enum_known_values,
		required,
		error_msg,
		file_not_found
	};

	struct LIBARGS_API base_translator {
		virtual ~base_translator();
		base_translator();
		base_translator(base_translator const&) = delete;
		base_translator(base_translator&&) = delete;
		base_translator& operator=(base_translator const&) = delete;
		base_translator& operator=(base_translator&&) = delete;
		virtual std::string operator()(lng id,
		                               std::string_view arg1 = {},
		                               std::string_view arg2 = {}) const = 0;
	};

	class LIBARGS_API null_translator : public base_translator {
	public:
		std::string operator()(lng id,
		                       std::string_view arg1,
		                       std::string_view arg2) const override;
	};
}  // namespace args