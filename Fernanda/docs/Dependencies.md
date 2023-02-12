# Dependencies

## Tree

- Main
	- MainWindow.h
		- ColorBar.h
			- Io.h (+ Path.h)
			- UserData.h (+ Path.h)
		- Editor.h
			- PlainTextEdit.h
				- Icon.h
				- KeyFilter.h
				- Layout.h
				- Path.h
				- Text.h (+ Version.h)
			- Style.h
				- Io.h (+ Path.h)
				- Text.h (+ Version.h)
			- UserData.h (+ Path.h)
		- Indicator.h
			- Icon.h
			- StatusBarButton.h
			- Text.h (+ Version.h)
			- UserData.h (+ Path.h)
		- Pane.h
			- Delegate.h
				- Icon.h
				- Index.h
			- Io.h (+ Path.h)
			- Text.h (+ Version.h)
		- Preview.h
			- Layout.h
			- UserData.h (+ Path.h)
		- Resource.h
			- Path.h
		- Splitter.h
			- UserData.h (+ Path.h)
		- Story.h
			- Archiver.h
				- Io.h (+ Path.h)
				- UserData.h (+ Path.h)
			- Dom.h
				- Io.h (+ Path.h)
			- Sample.h
				- Io.h (+ Path.h)
			- Text.h (+ Version.h)
		- Tool.h
			- Icon.h
			- Popup.h
				- Text.h (+ Version.h)
			- StatusBarButton.h
			- UserData.h (+ Path.h)
	- StartCop.h

## List

### Archiver.h
```
#include "Io.h"
#include "UserData.h"

#include "bit7z/include/bit7z.hpp"
#include "bit7z/include/bitarchiveeditor.hpp"
#include <QTemporaryDir>

#include <map>
#include <vector>
```

### ColorBar.h
```
#include "Io.h"
#include "UserData.h"

#include <QProgressBar>
#include <QTimeLine>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
```

### Delegate.h
```
#include "Icon.h"
#include "Index.h"

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

### Dom.h
```
#include "Io.h"

#include <QDomDocument>
#include <QDomElement>
#include <QUuid>
#include <QVector>
```

### Editor.h
```
#include "PlainTextEdit.h"
#include "Style.h"
#include "UserData.h"

#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QShortcut>
#include <QSizePolicy>
#include <Qt>
#include <QTextOption>
#include <QTimer>
```

### Icon.h
```
#include <QString>
```

### Index.h
```
#include <QModelIndex>
#include <QString>
#include <Qt>
#include <QVariant>

#include <type_traits>
```

### Indicator.h
```
#include "Icon.h"
#include "StatusBarButton.h"
#include "Text.h"
#include "UserData.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
```

### Io.h
```
#include "Path.h"

#include <QFile>
#include <QIODevice>
#include <QTextStream>

#include <optional>
#include <utility>
```

### KeyFilter.h
```
#include <QChar>
#include <QKeyEvent>
#include <QTextCursor>
#include <QVector>

#include <unordered_set>
```

### Layout.h
```
#include <QStackedLayout>
#include <QVector>
#include <QWidget>
```

### MainWindow.h
```
#include "ColorBar.h"
#include "Editor.h"
#include "Indicator.h"
#include "Pane.h"
#include "Preview.h"
#include "Resource.h"
#include "Splitter.h"
#include "Story.h"
#include "Tool.h"

#include <QCloseEvent>
#include <QCoreApplication>
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

### Pane.h
```
#include "Delegate.h"
#include "Io.h"
#include "Text.h"

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

### Path.h
```
#include <QDir>
#include <QtGlobal>
#include <QString>
#include <qsystemdetection.h>
#include <QVariant>

#include <filesystem>
#include <string>
#include <type_traits>
```

### PlainTextEdit.h
```
#include "Icon.h"
#include "KeyFilter.h"
#include "Layout.h"
#include "Path.h"
#include "Text.h"

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

### Popup.h
```
#include "Text.h"

#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>
```

### Preview.h
```
#include "Layout.h"
#include "UserData.h"

#include <QDesktopServices>
#include <QEvent>
#include <QObject>
#include <Qt>
#include <QUrl>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QWidget>

#include <memory>
```

### Resource.h
```
#include "Path.h"

#include <QDirIterator>
#include <QVector>

#include <algorithm>
```

### Sample.h
```
#include "Io.h"

#include <QDirIterator>
#include <QStringList>
#include <QVector>
```

### Splitter.h
```
#include "UserData.h"

#include <QByteArray>
#include <QSplitter>
#include <QVector>
#include <QWidget>
```

### StartCop.h
```
#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <Qt>
#include <QString>
//#include <qsystemdetection.h>
#include <QWidget>

/*#ifdef Q_OS_WINDOWS

#include <Windows.h>
#include <WinUser.h>

#endif*/
```

### StatusBarButton.h
```
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QPushButton>
```

### Story.h
```
#include "Archiver.h"
#include "Dom.h"
#include "Sample.h"
#include "Text.h"

#include <QPrinter>
#include <QStandardItem>
#include <QTextDocument>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
```

### Style.h
```
#include "Io.h"
#include "Text.h"

#include <QAction>
#include <QActionGroup>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
```

### Text.h
```
#include "Version.h"

#include <QRegularExpression>
#include <QString>
#include <QStringList>
```

### Tool.h
```
#include "Icon.h"
#include "Popup.h"
#include "StatusBarButton.h"
#include "UserData.h"

#include <QMainWindow>
#include <QMouseEvent>
#include <Qt>
#include <QTimer>

#ifdef Q_OS_WINDOWS

#include <Windows.h>

#endif

#include <optional>
```

### UserData.h
```
#include "Path.h"

#include <QCoreApplication>
#include <QFile>
#include <QRect>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QVariant>

#include <ctime>
```

### Version.h
```
```
