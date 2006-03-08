//@line 36 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/sanitize.js"

var gSanitizeDialog = {
  sanitizePasswords: function ()
  {
    var preference = document.getElementById("privacy.item.passwords");
    var promptPref = document.getElementById("privacy.sanitize.promptOnSanitize");
    if (preference.value)
      promptPref.value = true;
    promptPref.disabled = preference.value;
    return undefined;
  },
  
  onReadSanitizePasswords: function ()
  {
    this.sanitizePasswords();
    return undefined;
  }
};

