<html>
<head>
<link rel=stylesheet type="text/css" href="format.css">
</head>

<body text='#000000' bgcolor='#AAAABE'>

<?php
 include ("limez.ini");
 include ("limez.lib");

 if ($type == "dblist")
   $s = $lname;
 else
   $s = $lname . " " . $fname;

 if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "LADD $type $s") == 1)
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

   echo "<script language=\"JavaScript\">\n";
   echo "<!-- \n";
   echo "parent.left.location.reload() \n";
   echo "parent.right.location.href=\"blank.html\" \n";
   echo "//--> \n";
   echo "</script>";

 }
?>

</body>
</html>
