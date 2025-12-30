const fs = require("fs");
let content = fs.readFileSync("src/App.tsx", "utf8");
const handleImportCode = `const handleImport = () => {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = ".json";
    input.onchange = async (e) => {
      const file = (e.target as HTMLInputElement).files?.[0];
      if (\!file || \!currentTeam) return;
      try {
        const text = await file.text();
        await importData(currentTeam.id, text);
        message.success("Data imported successfully");
      } catch (err) {
        message.error("Failed to import data");
      }
    };
    input.click();
  };`;
content = content.replace("undefined", handleImportCode);
fs.writeFileSync("src/App.tsx", content);
console.log("Fixed\!");
