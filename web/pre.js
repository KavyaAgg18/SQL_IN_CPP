var Module = Module || {};

(function () {
  function stripAnsi(text) {
    return String(text).replace(/\x1b\[[0-9;]*m/g, "");
  }

  function writeToTerminal(text) {
    if (typeof window !== "undefined" && typeof window.appendTerminalText === "function") {
      window.appendTerminalText(stripAnsi(text));
    }
  }

  Module.print = function (text) {
    writeToTerminal(String(text) + "\n");
  };

  Module.printErr = function (text) {
    writeToTerminal(String(text) + "\n");
  };

  Module.onAbort = function (reason) {
    writeToTerminal("\n[Runtime abort] " + String(reason || "Unknown error") + "\n");
  };

  Module.stdout = function (chr) {
    if (chr === null || chr === undefined) return;
    writeToTerminal(String.fromCharCode(chr));
  };

  Module.stderr = function (chr) {
    if (chr === null || chr === undefined) return;
    writeToTerminal(String.fromCharCode(chr));
  };
})();
