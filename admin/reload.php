<html>
<link rel=stylesheet type="text/css" href="format.css">
<body text='#000000' bgcolor='#AAAABE'>
<?php
 include ("limez.ini");
 include ("limez.lib");

 if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "LRLD") == 1)
 {
   $x = fgets ($fp,128);

   // response in status frame
   echo "<b>Server Response: </b><br>$x";

   endcon ($fp);
 }

?>
</body>
</html>
