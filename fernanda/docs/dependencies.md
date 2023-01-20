# Dependencies

## Tree

- main
	- fernanda.h
		- colorbar.h
			- io.h (+ path.h)
			- userdata.h (+ path.h)
		- editor.h
			- plaintextedit.h
				- icon.h
				- keyfilter.h
				- layout.h
				- path.h
				- text.h (+ version.h)
			- style.h
				- io.h (+ path.h)
				- text.h (+ version.h)
			- userdata.h (+ path.h)
		- indicator.h
			- text.h (+ version.h)
			- userdata.h (+ path.h)
		- pane.h
			- delegate.h
				- icon.h
				- index.h
			- io.h (+ path.h)
			- text.h (+ version.h)
		- resource.h
			- path.h
		- splitter.h
			- userdata.h (+ path.h)
		- story.h
			- archiver.h
				- io.h (+ path.h)
				- userdata.h (+ path.h)
			- dom.h
				- io.h (+ path.h)
			- sample.h
				- io.h (+ path.h)
			- text.h (+ version.h)
		- tool.h
			- icon.h
			- popup.h
				- text.h (+ version.h)
			- userdata.h (+ path.h)
	- startcop.h

## List

### archiver.h
```
#include "bit7z/include/bit7z.hpp"
#include "bit7z/include/bitarchiveeditor.hpp"

#include "io.h"
#include "userdata.h"

#include <map>
#include <vector>

#include <QTemporaryDir>
```

### colorbar.h
```
#include "io.h"
#include "userdata.h"

#include <QProgressBar>
#include <QTimeLine>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
```

### delegate.h
```
#include "icon.h"
#include "index.h"

#include <QColor>
#include <QFont>
#include <QObject>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QWidget>
```

### dom.h
```
#include "io.h"

#include <QDomDocument>
#include <QDomElement>
#include <QUuid>
#include <QVector>
```

### editor.h
```
#include "plaintextedit.h"
#include "style.h"
#include "userdata.h"

#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QShortcut>
#include <QSizePolicy>
#include <Qt>
#include <QTextOption>
#include <QTimer>
```

### fernanda.h
```
#include "colorbar.h"
#include "editor.h"
#include "indicator.h"
#include "pane.h"
#include "resource.h"
#include "splitter.h"
#include "story.h"
#include "tool.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMap>
#include <QMenuBar>
#include <QMoveEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QShowEvent>
#include <QSlider>
#include <QStatusBar>
#include <QUrl>
#include <QWidgetAction>
```

### icon.h
```
#include <QString>
```

### index.h
```
#include <type_traits>

#include <QModelIndex>
#include <QString>
#include <Qt>
#include <QVariant>
```

### indicator.h
```
#include "text.h"
#include "userdata.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
```

### io.h
```
#include "path.h"

#include <optional>
#include <utility>

#include <QFile>
#include <QIODevice>
#include <QTextStream>
```

### keyfilter.h
```
#include "unordered_set"

#include <QChar>
#include <QKeyEvent>
#include <QTextCursor>
#include <QVector>
```

### layout.h
```
#include <QStackedLayout>
#include <QVector>
#include <QWidget>
```

### pane.h
```
#include "delegate.h"
#include "io.h"
#include "text.h"

#include <QAbstractItemView>
#include <QAction>
#include <QContextMenuEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QInputDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QPoint>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>
```

### path.h
```
#include <filesystem>
#include <string>
#include <type_traits>

#include <qsystemdetection.h>

#include <QDir>
#include <QtGlobal>
#include <QString>
```

### plaintextedit.h
```
#include "icon.h"
#include "keyfilter.h"
#include "layout.h"
#include "path.h"
#include "text.h"

#include <QAbstractSlider>
#include <QColor>
#include <QContextMenuEvent>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QLatin1Char>
#include <QObject>
#include <QPainter>
#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSize>
#include <QTextBlock>
#include <QTextFormat>
#include <QTextEdit>
#include <QWheelEvent>
```

### popup.h
```
#include "text.h"

#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>
```

### resource.h
```
#include "path.h"

#include <algorithm>

#include <QDirIterator>
#include <QVector>
```

### sample.h
```
#include "io.h"

#include <QDirIterator>
#include <QStringList>
#include <QVector>
```

### splitter.h
```
#include "userdata.h"

#include <QByteArray>
#include <QSplitter>
#include <QVector>
#include <QWidget>
```

### startcop.h
```
#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <Qt>
#include <QString>
#include <QWidget>
```

### story.h
```
#include "archiver.h"
#include "dom.h"
#include "sample.h"
#include "text.h"

#include <QStandardItem>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
```

### style.h
```
#include "io.h"
#include "text.h"

#include <QAction>
#include <QActionGroup>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
```

### text.h
```
#include "version.h"

#include <QRegularExpression>
#include <QString>
#include <QStringList>
```

### tool.h
```
#include "icon.h"
#include "popup.h"
#include "userdata.h"

#include <optional>

#ifdef Q_OS_WINDOWS

#include <Windows.h>

#endif

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
```

### userdata.h
```
#include "path.h"

#include <time.h>

#include <QCoreApplication>
#include <QFile>
#include <QRect>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QVariant>
```

### version.h
```
```