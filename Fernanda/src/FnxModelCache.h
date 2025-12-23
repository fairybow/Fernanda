/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QDomElement>
#include <QHash>
#include <QList>
#include <QString>

#include "Debug.h"
#include "Fnx.h"

namespace Fernanda {

class FnxModelCache
{
public:
    FnxModelCache() = default;
    virtual ~FnxModelCache() { TRACER; }

private:
    // Non-copyable (contains mutable state tied to specific DOM)
    FnxModelCache(const FnxModelCache&) = delete;
    FnxModelCache& operator=(const FnxModelCache&) = delete;


};

} // namespace Fernanda
