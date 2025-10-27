/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

namespace Fernanda {

// TODO: Properly integrate again if needed
enum class SaveChoice
{
    Cancel = 0,
    Save,
    Discard
};

// TODO: Properly integrate again if needed
enum class SaveResult
{
    NoOp = 0,
    Success,
    Fail
};

namespace Enum {

    inline QString toQString(SaveResult saveResult) noexcept
    {
        switch (saveResult) {
        default:
        case SaveResult::NoOp:
            return "SaveResult::NoOp";
        case SaveResult::Success:
            return "SaveResult::Success";
        case SaveResult::Fail:
            return "SaveResult::Fail";
        }
    }

    inline QString toQString(SaveChoice saveChoice) noexcept
    {
        switch (saveChoice) {
        default:
        case SaveChoice::Cancel:
            return "SaveChoice::Cancel";
        case SaveChoice::Save:
            return "SaveChoice::Save";
        case SaveChoice::Discard:
            return "SaveChoice::Discard";
        }
    }

} // namespace Enum

} // namespace Fernanda
