/*  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

// story.h, Fernanda

#pragma once

#include "archiver.h"
#include "dom.h"
#include "sample.h"
#include "text.h"

#include <QPrinter>
#include <QStandardItem>
#include <QTextDocument>
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
    enum class To {
        Directory,
        PDF,
        PlainText
    };

    struct TotalCounts {
        int lines = 0;
        int words = 0;
        int characters = 0;
    };

    Story(StdFsPath filePath, Mode mode = Mode::Normal);

    QVector<QStandardItem*> items();
    const QString tempSaveOld_openNew(QString newKey, QString oldText = nullptr);
    void autoTempSave(QString text);
    QStringList edits(QString currentText);
    bool cut(QString key);
    void save(QString text = nullptr);
    const TotalCounts totalCounts();
    void exportTo(StdFsPath path, To type);

    bool hasChanges() { return (!editedKeys.isEmpty() || dom->hasChanges()); }
    const QString key() { return activeKey; }
    const QString devGetDom(Dom::Document document = Dom::Document::Current) { return dom->string(document); }
    QVector<Io::ArchiveRename> devGetRenames() { return dom->renames(); }
    const QStringList devGetEditedKeys() { return editedKeys; }
    const StdFsPath devGetActiveTemp() { return UserData::doThis(UserData::Operation::GetActiveTemp) / name<StdFsPath>(); }
    void setItemExpansion(QString key, bool isExpanded) { dom->write(key, isExpanded, Dom::Write::Expanded); }
    void move(QString pivotKey, QString fulcrumKey, Io::Move position) { dom->move(pivotKey, fulcrumKey, position); }
    void rename(QString newName, QString key) { dom->rename(newName, key); }
    void add(QString newName, Path::Type type, QString parentKey) { dom->add(newName, type, parentKey); }

    template<typename T>
    inline const T name() { return Path::getName<T>(activeArchivePath); }

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
    void bak();
    const QString readAllForExport();
    void toPdf(StdFsPath path);

    bool isEdited(QString key) { return (editedKeys.contains(key)); }
    void toPlainText(StdFsPath path) { Io::writeFile(path, readAllForExport()); }
};

// story.h, Fernanda
