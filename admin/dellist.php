<html>
<head>
<link rel=stylesheet type="text/css" href="format.css">
<script language="JavaScript">
<!--
parent.right.location.href="blank.html";
window.setTimeout("parent.left.location.reload()",0);
window.setTimeout("parent.info.location.reload()",1000);
//-->
</script>
</head>

<body text='#000000' bgcolor='#AAAABE'>

<?php
 include ("limez.ini");
 include ("limez.lib");

 if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "LDEL list $list") == 1)
 {
   $x = fgets ($fp,128);

   // response in status frame
   if ($x[0]=="2")
   {
      echo "<b>Server Response: </b><br>$x<br>";
      // reread config
      sleep(1);
      fputs ($fp, "LRLD\r\n");
      $x = fgets ($fp,128);
      echo "$x<br>";
   }
   else
      echo "<b>Server Response: </b><br><font color=red>$x</font><br>";

   endcon ($fp);
 }
?>

</body>
</html>
