// main.cpp, Fernanda

#include "fernanda.h"
#include "startcop.h"

int main(int argc, char *argv[])
{
    StartCop guard("fernanda.app");
    if (guard.exists()) return 0;
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication::setDesktopSettingsAware(true);
    QApplication app(argc, argv);
    std::filesystem::path opener = std::filesystem::path();
    for (auto& arg : app.arguments())
        if (arg.endsWith(".story"))
            opener = Path::toStdFs(arg);
    Fernanda window(app.arguments().contains("-dev"), opener);
    {
        auto font = app.font();
        font.setStyleStrategy(QFont::PreferAntialias);
        font.setHintingPreference(QFont::HintingPreference::PreferNoHinting);
        font.setPointSizeF(9);
        app.setFont(font);
    }
    window.show();
    return app.exec();
}

// main.cpp, Fernanda
