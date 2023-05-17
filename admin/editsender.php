<html>
<head><title>Sender</title>
<link rel=stylesheet type="text/css" href="format.css">
</head>
<body onLoad="document.adddel.user.focus();" text='#000000' bgcolor='#ffffff'>
<br><center><font size=3><b>Sender of list '<?php echo $list; ?>'</b></font><center><br>

<table align="center" width="80%" border="0">
    <tr>
      <td width="80%" id="tbdkl">
        <div align="center"> <font face="Arial, Helvetica, sans-serif" id="lite"><b>
           sender</b></font></div>
      </td>
      <td width="20%" id="tbdkl">
        <div align="center"> <font face="Arial, Helvetica, sans-serif" id="lite"><b>
           action</b></font></div>
      </td>
    </tr>

<?php
 include ("limez.ini");
 include ("limez.lib");

 if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "LSHW sender $list") == 1)
 {
   // add all read entries in hash $main
   while(1)
   {
     $x = fgets ($fp,128);
     if (strncmp($x, $end, 4))
       break;
     $usr = substr($x,4);

     echo "<tr><td id=\"tbhel\"><tt><font id=\"dark\">$usr</font></tt></td>";
     echo "<td align=center id=\"tbhel\">";
     echo "<a href=\"changeuser.php?list=$list&action=delsender&user=$usr\" target=\"status\">";
     echo "<font id=\"ll\">Delete</a></font>";
     echo "</td></tr>";
   }

   // end table
   echo "</table>";

   // add or delete sender
   echo "<table align=center width=80%><tr><td align=center valign=middle height=20>";
   echo "<form name=\"adddel\" action=\"changeuser.php\" target=\"status\" method=post>";
   echo "<input type=hidden name=\"list\" value=\"$list\">";
   echo "<input type=text name=\"user\" size=40 maxlength=128></td>";
   echo "<td align=center valign=middle height=20>";
   echo "<input type=radio name=\"action\" value=\"addsender\" checked>Insert<br>";
   echo "<input type=radio name=\"action\" value=\"delsender\">Delete</td>";
   echo "<td align=center valign=middle height=20><input type=submit value=\"Go for it!\">";
   echo "</td></form></tr></table>";

   endcon ($fp);
 }



?>
