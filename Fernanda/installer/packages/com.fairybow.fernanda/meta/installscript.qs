function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        // Install VC++ Redistributable (silent, no restart, no undo)
        component.addOperation("Execute",
            "@TargetDir@\\vc_redist.x64.exe",
            "/install", "/quiet", "/norestart");

        // Start Menu shortcut (always)
        component.addOperation("CreateShortcut",
            "@TargetDir@\\Fernanda.exe",
            "@StartMenuDir@\\Fernanda.lnk",
            "workingDirectory=@TargetDir@",
            "iconPath=@TargetDir@\\Fernanda.exe",
            "iconId=0",
            "description=A text editor for drafting long-form fiction");

        // Desktop shortcut (conditional)
        var checkBox = component.userInterface("ShortcutOptions").desktopShortcutCheckBox;
        if (checkBox && checkBox.checked) {
            component.addOperation("CreateShortcut",
                "@TargetDir@\\Fernanda.exe",
                "@DesktopDir@\\Fernanda.lnk",
                "workingDirectory=@TargetDir@",
                "iconPath=@TargetDir@\\Fernanda.exe",
                "iconId=0",
                "description=A text editor for drafting long-form fiction");
        }

        // .fnx file association
        component.addOperation("RegisterFileType",
            "fnx",
            "@TargetDir@\\Fernanda.exe '%1'",
            "Fernanda Notebook",
            "application/x-fernanda-notebook",
            "@TargetDir@\\Fernanda.exe,0",
            "ProgId=Fernanda.Notebook");
    }
}
