function HighlightedString(str, range) {
  this.string = str;
  this.min = range[0];
  this.max = range[1];
}

function escapeHtml(unsafe) {
  return unsafe
       .replace(/&/g, "&amp;")
       .replace(/</g, "&lt;")
       .replace(/>/g, "&gt;")
       .replace(/"/g, "&quot;")
       .replace(/'/g, "&#039;");
}

HighlightedString.prototype.substring = function(a, b) {
  var mB = this.min < b ? this.min : b;
  var mA = this.max > a ? this.max : a;
  
  var before = a >= this.min ? '' : escapeHtml(this.string.substr(a, mB - a));
  var after  = b <= this.max ? '' : escapeHtml(this.string.substr(mA, b - mA));
  
  var hA = a < this.min ? mB : a;
  var hB = b > this.max ? mA : b;
  var inside = hA >= hB ? '' : '<font color="red">' + escapeHtml(this.string.substr(hA, hB - hA)) + '</font>';
  
  var pad = '';
  
  var padLength = b - a;
  if(a < this.min) padLength -= mB - a;
  if(b > this.max) padLength -= (b > this.string.length ? this.string.length : b) - mA;
  if(hA < hB) padLength -= hB - hA;
  
  for(var i = 0; i < padLength; ++i) pad += ' ';
  
  return before + inside + after + pad;
};