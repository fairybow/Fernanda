/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QByteArray>
#include <QByteArrayView>
#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <Coco/Path.h>

#include "models/TextFileModel.h"

// The warning before Suzuri opens a text file that isn't valid UTF-8. Suzuri
// reads only UTF-8, matching Obsidian, and does no conversion: accepting opens
// the file with every undecodable sequence shown as U+FFFD, and
// Vault::openModel saves it at once, so the file on disk becomes the UTF-8 the
// buffer shows. That immediate save is why accepting IS the loss, and why the
// wording says so plainly. A free function, so VaultWindow stays slim
namespace Suzuri::LossyOpenPrompt {

namespace Internal {

// Enough to judge a manuscript by; a binary under a text extension would
// otherwise hand the details pane megabytes of noise
inline constexpr qsizetype MAX_PREVIEW_LINES_ = 50;
inline constexpr qsizetype MAX_PREVIEW_LINE_CHARS_ = 200;

// Every line of data that doesn't decode cleanly, as "Line N: ..." in the
// decoded form the buffer will hold — U+FFFD where the originals were. Lines
// are found by splitting the raw bytes on 0x0A, which is safe because that
// byte never occurs inside a UTF-8 multibyte sequence or in a single-byte
// encoding like Windows-1252 (a UTF-16 file breaks this, but its preview reads
// as garbage, which is the right warning). An invalid sequence therefore never
// spans lines, so a file that fails the whole-file check always has at least
// one affected line here. A trailing '\r' is stripped; it can't complete a
// sequence, so stripping it never turns a bad line good
[[nodiscard]] inline QString affectedLines_(const QByteArray& data)
{
    QByteArrayView all(data);
    QStringList shown{};
    qsizetype affected_count = 0;
    qsizetype line_number = 0;

    for (qsizetype start = 0; start <= all.size();) {
        auto end = all.indexOf('\n', start);
        if (end < 0) {
            end = all.size();
        }

        auto line = all.sliced(start, end - start);
        ++line_number;
        start = end + 1;

        if (line.endsWith('\r')) {
            line.chop(1);
        }

        if (TextFileModel::isValidUtf8(line)) {
            continue;
        }

        ++affected_count;
        if (shown.size() >= MAX_PREVIEW_LINES_) {
            continue; // still counted, for the summary
        }

        auto text = QString::fromUtf8(line);

        // Truncate without splitting a surrogate pair
        if (text.size() > MAX_PREVIEW_LINE_CHARS_) {
            auto cut = MAX_PREVIEW_LINE_CHARS_;
            if (text.at(cut - 1).isHighSurrogate()) {
                --cut;
            }
            text = text.left(cut) + u'\u2026';
        }

        shown << QCoreApplication::translate("LossyOpenPrompt", "Line %1: %2")
                     .arg(line_number)
                     .arg(text);
    }

    QStringList parts{};

    parts << QCoreApplication::translate(
        "LossyOpenPrompt",
        "%n line(s) contain characters Suzuri can't read. They'll open as:",
        nullptr,
        static_cast<int>(affected_count));

    parts << QString{} << shown.join(u'\n');

    if (auto hidden = affected_count - shown.size(); hidden > 0) {
        parts << QString{}
              << QCoreApplication::translate(
                     "LossyOpenPrompt",
                     "\u2026and %n more.",
                     nullptr,
                     static_cast<int>(hidden));
    }

    return parts.join(u'\n');
}

} // namespace Internal

// True to open anyway. Cancel is both the default and the escape button, so
// Enter and Esc take the path that changes nothing — set explicitly, since
// the details button would otherwise be a candidate. The preview rides in
// setDetailedText: Qt's built-in "Show Details..." pane, read-only,
// scrollable, and selectable, with no widget of our own
[[nodiscard]] inline bool
confirm(const Coco::Path& relative, const QByteArray& data, QWidget* parent)
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowModality(Qt::WindowModal);

    // A file name is never markup — QMessageBox would otherwise sniff for it
    box.setTextFormat(Qt::PlainText);

    box.setWindowTitle(
        QCoreApplication::translate("LossyOpenPrompt", "Not UTF-8"));

    box.setText(
        QCoreApplication::translate(
            "LossyOpenPrompt",
            "\"%1\" isn't valid UTF-8. It was probably saved in an "
            "older encoding, such as Windows-1252.")
            .arg(relative.prettyQString()));

    box.setInformativeText(
        QCoreApplication::translate(
            "LossyOpenPrompt",
            "Suzuri reads only UTF-8. Opening this file saves it as UTF-8 "
            "right "
            "away, and every character Suzuri can't read (often curly quotes, "
            "dashes, and accented letters) is permanently replaced with "
            "\uFFFD.\n\n"
            "To keep those characters, cancel, then re-save the file as UTF-8 "
            "in "
            "another program first."));

    box.setDetailedText(Internal::affectedLines_(data));

    auto* open = box.addButton(
        QCoreApplication::translate("LossyOpenPrompt", "Open Anyway"),
        QMessageBox::AcceptRole);
    auto* cancel = box.addButton(QMessageBox::Cancel);

    box.setDefaultButton(cancel);
    box.setEscapeButton(cancel);
    box.exec();

    return box.clickedButton() == open;
}

} // namespace Suzuri::LossyOpenPrompt
