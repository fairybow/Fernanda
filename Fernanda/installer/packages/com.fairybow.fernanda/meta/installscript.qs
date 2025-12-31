function Component() {}

Component.prototype.createOperations = function() {
    component.createOperations();

    if (systemInfo.productType === "windows") {
        // Start Menu shortcut (TODO: Make optional)
        component.addOperation("CreateShortcut",
            "@TargetDir@/Fernanda.exe",
            "@StartMenuDir@/Fernanda.lnk");

        // .fnx file association
        component.addOperation("RegisterFileType",
            "fnx",
            "@TargetDir@/Fernanda.exe '%1'",
            "Fernanda Notebook",
            "application/x-fernanda-notebook",
            "@TargetDir@/Fernanda.exe,0",
            "ProgId=Fernanda.Notebook");
    }
}
