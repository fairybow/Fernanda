#pragma once

/// Find all SaveResult and SaveChoice uses and then put them in Enum and remove
/// headers where appropriate from files that used these enums (and replace with
/// this).

namespace Fernanda {

enum class SaveChoice
{
    Cancel = 0,
    Save,
    Discard
};

enum class SaveResult
{
    NoOp = 0,
    Success,
    Fail
};

} // namespace Fernanda
