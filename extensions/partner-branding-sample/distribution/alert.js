/**
 * Sample JScript for Windows Scripting Host to show a message box based on the
 * command line parameters
 *
 * Usage: wscript alert.js stuff goes here
 */
var WshShell = WScript.CreateObject("WScript.Shell");
var args = [];
for (var i = 0; i < WScript.Arguments.Length; ++i) {
    args.push(String(WScript.Arguments.Item(i)));
}
WshShell.Popup(args.join("\n"));
