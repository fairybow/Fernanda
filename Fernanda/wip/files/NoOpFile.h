#pragma once

#include "IFile.h"

class NoOpFile : public IFile
{
    Q_OBJECT

public:
    using IFile::IFile;
    virtual bool canEdit() const override { return false; }
};
