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
    using FsPath = std::filesystem::path;

public:
    enum class Op {
        Normal,
        Sample
    };

    Story(FsPath filePath, Op opt = Op::Normal);

    const QString devGetDom(Dom::Doc doc = Dom::Doc::Current);
    QVector<Io::ArcRename> devGetRenames();
    const QStringList devGetEditedKeys();
    const FsPath devGetActiveTemp();
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

    template<typename T>
    inline const T name()
    {
        return Path::getName<T>(activeArcPath);
    }

private:
    Archiver* archiver = new Archiver;
    Dom* dom = new Dom;

    FsPath activeArcPath;
    QString activeKey = nullptr;
    QString cleanText = nullptr;
    QStringList editedKeys;

    enum class AmendEdits {
        Add,
        Remove
    };

    void make(Op opt);
    const QString xml();
    void newXml();
    void newXml_recursor(QXmlStreamWriter& writer, FsPath rPath, FsPath rootPath = FsPath());
    QStandardItem* items_recursor(QXmlStreamReader& reader);
    void tempSave(QString key, QString text);
    const QString tempOpen(QString newKey);
    const FsPath tempPath(QString key);
    void amendEditsList(AmendEdits op, QString key = nullptr);
    bool isEdited(QString key);
    void bak();
};

// story.h, Fernanda
