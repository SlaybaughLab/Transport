#include "instrumentation/converter/string_color_pair_to_string.h"

namespace bart {

namespace instrumentation {

namespace converter {

namespace  {
using OutputTerm = StringColorPairToStringOutputTerm;
std::string default_output_format{"${COLOR_CODE}${STRING}${COLOR_RESET_CODE}"};
std::map<StringColorPairToStringOutputTerm, std::string>
    default_output_term_to_string_map{
    {OutputTerm::kColorCode,  "${COLOR_CODE}"},
    {OutputTerm::kString,     "${STRING}"},
    {OutputTerm::kColorReset, "${COLOR_RESET_CODE}"}};

} // namespace

StringColorPairToString::StringColorPairToString()
: ToStringConverter<std::pair<std::string, utility::Color>, OutputTerm>(
    default_output_format, default_output_term_to_string_map) {}

std::string StringColorPairToString::Convert(
    const std::pair<std::string, Color> &to_convert) const {
  std::string output = output_format_;

  std::string string_string = output_term_to_string_map_.at(OutputTerm::kString);
  if (auto index = output.find(string_string); index != std::string::npos)
    output.replace(index, string_string.size(), to_convert.first);
  std::string color_string = output_term_to_string_map_.at(
      OutputTerm::kColorCode);
  if (auto index = output.find(color_string); index != std::string::npos) {
    output.replace(index, color_string.size(),
                   utility::to_string(to_convert.second));
  }
  std::string reset_string = output_term_to_string_map_.at(
      OutputTerm::kColorReset);
  if (auto index = output.find(reset_string); index != std::string::npos) {
    output.replace(index, reset_string.size(),
                   utility::to_string(Color::kReset));
  }
  return output;
}

} // namespace converter

} // namespace instrumentation

} // namespace bart
