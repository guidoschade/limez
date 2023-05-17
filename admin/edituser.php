<html>
<head><title>User</title>
<link rel=stylesheet type="text/css" href="format.css">
<script language="JavaScript">
<!--
function load(num,list)
{
  var x = "edituser.php?num=" + num + "&list=" + list;
  parent.frames[1].location.href = x;
}
//-->
</script>
</head>
<body onLoad="document.adddel.user.focus();" text='#000000' bgcolor='#ffffff'>

<?php
 include ("limez.ini");
 include ("limez.lib");

 if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "LSHW usercount $list") == 1)
 {
   $users = 20;
   $x = substr(fgets ($fp,128),4);
   $s = ($num - 1) * $users + 1;
   $e = $s + $users - 1;
   $pv = $num -1;
   $nx = $num +1;
   if (($x % $users) == 0)
     $last = (int) (($x) / $users);
   else
     $last = (int) (($x) / $users) + 1;
   if ($e >= $x)
   {
     $nx = 0;
     $e = $x;
   }

   if ($x > $users)
   {
     echo "<table width=80% align=center border=0><tr><td>";

     echo "<form name=\"first\">";
     echo "<input type=button value=\"first\" onClick=\"load('1','$list')\"></td></form><td>";

     if ($pv != 0)
     {
       echo "<form name=\"prev\">";
       echo "<input type=button value=\"prev page\" onClick=\"load('$pv','$list')\">";
     }
     else echo "&nbsp;";

     echo "</td></form><td align=center valign=middle><th><font size=3>User ($s - $e) of list '$list' (total: $x)</font></th>";
     echo "<td>";

     if ($nx != 0)
     {
       echo "<form name=\"next\">";
       echo "<input type=button value=\"next page\" onClick=\"load('$nx','$list')\">";
     }
     else echo "&nbsp;";

     echo "</td></form><td><form name=\"last\">";
     echo "<input type=button value=\"last\" onClick=\"load('$last','$list')\">";

     echo "</td></form></tr>";
     echo "</table>";
   }
   else
     echo "<br><center><font size=3><b>User of list '$list' (total: $x)</b></font><center><br>";
?>

<table align="center" width="80%" border="0">
    <tr>
      <td width="80%" id="tbdkl" colspan=2>
        <div align="center"> <font face="Arial, Helvetica, sans-serif" id="lite"><b>
           user</b></font></div>
      </td>
      <td width="20%" id="tbdkl">
        <div align="center"> <font face="Arial, Helvetica, sans-serif" id="lite"><b>
           action</b></font></div>
      </td>
    </tr>

<?php

   fputs ($fp, "LSHW user $list $num\r\n");

   // show users
   while(1)
   {
     $x = fgets ($fp,128);
     if (strncmp($x, $end, 4))
       break;
     $usr = substr($x,4);
     echo "<tr><td id=\"tbhel\" colspan=2><tt><font id=\"dark\">$usr</font></tt></td>";
     echo "<td align=center id=\"tbhel\">";
     echo "<a href=\"changeuser.php?list=$list&action=xdel&user=$usr\" target=\"status\">";
     echo "<font id=\"ll\">Delete</a></font>";
     echo "</td></tr>";
   }

   // end table
   // echo "</table>";

   // add or delete user
   // echo "<table align=center width=80% border=0>":
   echo "<tr><td align=center valign=middle height=20>";
   echo "<form name=\"adddel\" action=\"changeuser.php\" target=\"status\" method=post>";
   echo "<input type=hidden name=\"list\" value=\"$list\">";
   echo "<input type=text name=\"user\" size=40 maxlength=128></td>";
   echo "<td align=center valign=middle height=20>";
   echo "<input type=radio name=\"action\" value=\"xadd\" checked>Insert<br>";
   echo "<input type=radio name=\"action\" value=\"xdel\">Delete</td>";
   echo "<td align=center valign=middle height=20><input type=submit value=\"Go for it!\">";
   echo "</td></form></tr></table>";

   endcon ($fp);
 }



?>
