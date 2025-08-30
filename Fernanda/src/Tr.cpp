#include <QString>

#include "Application.h"
#include "Tr.h"
#include "Version.h"

namespace Fernanda {

QString tr(const char* sourceText, const char* disambiguation, int n)
{
    return Application::translate(
        VERSION_APP_NAME_STRING,
        sourceText,
        disambiguation,
        n);
}

} // namespace Fernanda
