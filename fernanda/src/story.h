// story.h, Fernanda

#pragma once

#include "archiver.h"
#include "dom.h"
#include "sample.h"
#include "text.h"

#include <QStandardItem>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

class Story
{
    using StdFsPath = std::filesystem::path;

public:
    enum class Mode {
        Normal,
        Sample
    };

    struct TotalCounts {
        int lines = 0;
        int words = 0;
        int characters = 0;
    };

    Story(StdFsPath filePath, Mode mode = Mode::Normal);

    const QString devGetDom(Dom::Document document = Dom::Document::Current);
    QVector<Io::ArchiveRename> devGetRenames();
    const QStringList devGetEditedKeys();
    const StdFsPath devGetActiveTemp();
    QVector<QStandardItem*> items();
    const QString key();
    const QString tempSaveOld_openNew(QString newKey, QString oldText = nullptr);
    void autoTempSave(QString text);
    QStringList edits(QString currentText);
    bool hasChanges();
    void setItemExpansion(QString key, bool isExpanded);
    void move(QString pivotKey, QString fulcrumKey, Io::Move position);
    void rename(QString newName, QString key);
    void add(QString newName, Path::Type type, QString parentKey);
    bool cut(QString key);
    void save(QString text = nullptr);
    const TotalCounts totalCounts();

    template<typename T>
    inline const T name()
    {
        return Path::getName<T>(activeArchivePath);
    }

private:
    Archiver* archiver = new Archiver;
    Dom* dom = new Dom;

    StdFsPath activeArchivePath;
    QString activeKey = nullptr;
    QString cleanText = nullptr;
    QStringList editedKeys;

    enum class AmendEdits {
        Add,
        Remove
    };

    void make(Mode mode);
    const QString xml();
    void newXml();
    void newXml_recursor(QXmlStreamWriter& writer, StdFsPath readPath, StdFsPath rootPath = StdFsPath());
    QStandardItem* items_recursor(QXmlStreamReader& reader);
    void tempSave(QString key, QString text);
    const QString tempOpen(QString newKey);
    const StdFsPath tempPath(QString key);
    void amendEditsList(AmendEdits operation, QString key = nullptr);
    bool isEdited(QString key);
    void bak();
};

// story.h, Fernanda
