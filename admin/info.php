<html>
<head><title>Info</title>
<link rel=stylesheet type="text/css" href="format.css">
</head>
<body onLoad="setTimeout('self.location.reload()',300000)" text='#000000' bgcolor='#000000'>

<?php
 include ("limez.ini");
 include ("limez.lib");

 if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "LSHW info") == 1)
 {
   // add all read entries in hash $info
   while(1)
   {
     $x = fgets ($fp,128);
     if (strncmp($x, $end, 4))
       break;

     $info[] = explode("\"", substr($x,4));
   }

   // show selected server informations
   echo "<table border=0 width=90% align=center bgcolor=#000000>";
   reset($info);
   $z = current($info);
   while ($z)
   {
     if (!strcmp($z[1], "UPTIME"))
     {
       echo "<tr><td id=\"tbinfo\"><font face=\"Arial, Helvetica, sans-serif\" id=\"lite\"><b>";
       echo "uptime</b></td><td id=\"tbinf2\">$z[3]</td></tr>";
     }
     if (!strcmp($z[1], "VERSION"))
     {
       echo "<tr><td id=\"tbinfo\"><font face=\"Arial, Helvetica, sans-serif\" id=\"lite\"><b>";
       echo "version</b></td><td id=\"tbinf2\">$z[3]</td></tr>";
     }
     if (!strcmp($z[1], "SERVER_IP"))
     {
       echo "<tr><td id=\"tbinfo\"><font face=\"Arial, Helvetica, sans-serif\" id=\"lite\"><b>";
       echo "server</b></td><td id=\"tbinf2\">$z[3]</td></tr>";
     }
     if (!strcmp($z[1], "DOMAIN"))
     {
       echo "<tr><td id=\"tbinfo\"><font face=\"Arial, Helvetica, sans-serif\" id=\"lite\"><b>";
       echo "domain</b></td><td id=\"tbinf2\">$z[3]</td></tr>";
     }
     $z = next($info);
   }
   echo "</table>";

   endcon ($fp);
 }
?>

</body>
</html>
