#include <string>
#include <vector>

namespace nv_helpers_gl
{
    struct IncludeEntry {
      std::string   name;
      std::string   filename;
      std::string   content;
    };

    typedef std::vector<IncludeEntry> IncludeRegistry;

    std::string manualInclude (
        std::string const & filenameorig,
        std::string const & source,
        std::string const & prepend,
        const std::vector<std::string>& directories,
        const IncludeRegistry &includes);
}
