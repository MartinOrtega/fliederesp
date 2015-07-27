boolean isNumber(String tString) {
  String tBuf;
  boolean decPt = false;

  if (tString.charAt(0) == '+' || tString.charAt(0) == '-') tBuf = &tString[1];
  else tBuf = tString;

  for (int x = 0; x < tBuf.length(); x++)
  {
    if (tBuf.charAt(x) == '.') {
      if (decPt) return false;
      else decPt = true;
    }
    else if (tBuf.charAt(x) < '0' || tBuf.charAt(x) > '9') return false;
  }
  return true;
}

//function to extract decimal part of float
long getDecimal(float val) {
  int intPart = int(val);
  long decPart = 1000 * (val - intPart); //I am multiplying by 1000 assuming that the foat values will have a maximum of 3 decimal places
  //Change to match the number of decimal places you need
  if (decPart > 0)return (decPart);       //return the decimal part of float number if it is available
  else if (decPart < 0)return ((-1) * decPart); //if negative, multiply by -1
  else if (decPart = 0)return (00);       //return 0 if decimal part of float number is not available
}
