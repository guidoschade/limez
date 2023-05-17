<html>
<head><title>LiMez Config</title>
<link rel=stylesheet type="text/css" href="format.css">
<script language="JavaScript">
<!--
function chkinput(formname)
{
  if (document.forms[formname].newval.value == "")
  {
    alert ("Please enter a value");
    document.forms[formname].newval.focus();
    return false;
  }
}
function chkchosen()
{
  var x = document.add.key.options[document.add.key.options.selectedIndex].value;
  if (x == 'oops')
  {
    alert ("Please choose a key",document.add.key.value);
    document.add.key.focus();
    return false;
  }
  if (document.add.value.value == "")
  {
    alert ("Please enter a value");
    document.add.value.focus();
    return false;
  }  
}
function doJump(description)
{
  var key = description.options[description.selectedIndex].value;
  if (key != "oops") 
  {
    var item = "describekey.php?key=" + key;
    parent.frames[3].location.href=item;
  }
  else
    parent.frames[3].location.href="status.php";
}
//-->
</script>
</head>
<body text='#000000' bgcolor='#ffffff'>
<center><font size=3><b>

<?php
  if ($list == "limez")
    echo "Server Configuration";
  else
    echo "Configuration of list '$list'";
?>

</b></font><center><br>

<table align="center" width="90%" border="0" cellpadding="0" cellspacing="0">
    <tr>
      <td height=20 width="35%" id="tbdkl">
        <div align="center"> <font face="Arial, Helvetica, sans-serif" id="lite">
           <b>key</b> (click for help)</font></div>
      </td>
      <td height=20 width="35%" id="tbdkl">
        <div align="center"> <font face="Arial, Helvetica, sans-serif" id="lite">
           <b>value</b> (default = * or bright background)</font></div>
      </td>
      <td height=20 width="30%" id="tbdkl" colspan=3>
        <div align="center"> <font face="Arial, Helvetica, sans-serif" id="lite">
           <b>action</b></font></div>
      </td>
    </tr>
    <tr bgcolor=#ffffff><td height=2></td></tr>

<?php
 include ("limez.ini");
 include ("limez.lib");

 function check($n,$r)
 {
   if ($n == $r)
     return "selected";
 }

 function cmp ($a, $b) {
     return strcmp($a[1],$b[1]);
 }

 if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "LSHW config $list") == 1)
 {
   // add all read entries in hash $main
   while(1)
   {
     $x = fgets ($fp,128);
     if (strncmp($x, $end, 4))
       break;

     $main[] = explode("\"", substr($x,4));
   }

   // get main config keyz
   if ($list == "limez")
     fputs ($fp, "LSHW mainkeys\r\n");
   else
     fputs ($fp, "LSHW listkeys\r\n");

   while(1)
   {
     $x = fgets ($fp,128);
     if (strncmp($x, $end, 4))
       break;

     $keyz[] = explode("\"", substr($x,4));
   }

   $ccount = count($main);
   $kcount = count($keyz);
   for ($y = 0; $y<$kcount; $y++)
   {
     $keyz[$y][2] = "true";

     for ($x = 0; $x<$ccount; $x++)
     {
       if (!strcmp($keyz[$y][1], $main[$x][1]))
       {
         if (!strcmp($keyz[$y][3], $main[$x][3]))
           $main[$x][2]="true";
         else
           $main[$x][2]="false";

         if (!($keyz[$y][5] & 2) || !($keyz[$y][5] & 1))
           $keyz[$y][2] = "false";
       }
     }
   }

   // show all config entries
   usort($main, "cmp");   
   reset($main);
   $z = current($main);
   $nr = 1;
   while ($z)
   {
     if (!strncmp($z[1], "mailinglist", 10) || !strncmp($z[1], "db_", 3) || !strcmp($z[1], "server_web_pass"))
     {
       $z = next($main);
       continue;
     }
 
     if (!($z[5] & 0x20) && !($z[5] & 0x08))
     {
       echo "<tr><td height=20 id=\"tbhel\"><tt><font id=\"dark\">\n";
       echo "<a href=\"describekey.php?key=$z[1]\" target=\"status\">$z[1]</a></font></tt></td>\n";

       if (!strcmp($z[2], "true"))
         echo "<td height=20 id=\"tbhel\">";
       else
         echo "<td height=20 id=\"tbhel2\">";

       echo "\n<form name=\"change$nr\" action=\"changeconf.php\" ";
       echo "target=\"status\" method=get ";
       echo "onSubmit=\"return chkinput('change$nr')\">\n";
       echo "<font id=\"dark\">\n";
       echo "<input type=hidden name=\"action\" value=\"change\">\n";
       echo "<input type=hidden name=\"list\" value=\"$list\">\n";
       echo "<input type=hidden name=\"key\" value=\"$z[1]\">\n";
       echo "<input type=hidden name=\"value\" value=\"$z[3]\">\n";
       $nr++; 

       $found = 0;
       if (!strcmp($z[1], "debug_level"))
       {
         $found = 1;
         echo "\n<select name=\"newval\" size=1>";
         echo "<option value='0'".check(0,$z[3]).">0 (no warnings/errors)\n";
         echo "<option value='1'".check(1,$z[3]).">1 (only major errors)\n";
         echo "<option value='2'".check(2,$z[3]).">2 (errors/warnings)\n";
         echo "<option value='3'".check(3,$z[3]).">* 3 (errors/warnings/info)\n";
         echo "<option value='4'".check(4,$z[3]).">4 (plus detailed info)\n";
         echo "<option value='5'".check(5,$z[3]).">5 (more detailed info)\n";
         echo "<option value='6'".check(6,$z[3]).">6 (debugging information)\n";
         echo "<option value='7'".check(7,$z[3]).">7 (more debugging stuff)\n";
         echo "<option value='8'".check(8,$z[3]).">8 (much more debugging)\n";
         echo "<option value='9'".check(9,$z[3]).">9 (maximum debug level)\n";
         echo "</select>";
       }

       if (!strcmp($z[1], "list_send"))
       {
         $found = 1;
         echo "<select name=\"newval\" size=1>";
         echo "<option value='open'".check("open",$z[3]).">open (everybody may send)\n";
         echo "<option value='closed'".check("closed",$z[3]).">closed (nobody is allowed to broadcast)\n";
         echo "<option value='sender'".check("sender",$z[3]).">sender (only people who are sender)\n";
         echo "<option value='subscriber'".check("subscriber",$z[3]).">* subscriber (all subscribed people)\n";

         echo "</select>";
       }

       if (!strcmp($z[1], "list_subscribe") || !strcmp($z[1], "list_unsubscribe"))
       {
         $found = 1;
         echo "<select name=\"newval\" size=1>";
         echo "<option value='open'".check("open",$z[3]).">* open (everybody may add/del himself)\n";
         echo "<option value='closed'".check("closed",$z[3]).">closed (nobody is allowed)\n";
         echo "<option value='anyone'".check("anyone",$z[3]).">anyone (anyone may add/del anyone)\n";
         echo "<option value='approve'".check("approve",$z[3]).">approve (only with password)\n";
         echo "</select>";
       }

       if (!strcmp($z[1], "list_info") || !strcmp($z[1], "list_stat"))
       {
         $found = 1;
         echo "<select name=\"newval\" size=1>";
         echo "<option value='open'".check("open",$z[3]).">open (everybody)\n";
         echo "<option value='closed'".check("closed",$z[3]).">* closed (nobody)\n";
         echo "<option value='approve'".check("approve",$z[3]).">approve (only with password)\n";
         echo "</select>";
       }

       if ($found == 0)
         echo "<input type=text name=\"newval\" size=25 maxlength=64 value=\"$z[3]\">\n";
         echo "</font></td>\n";

       echo "<td align=center valign=middle id=\"tbhel\">\n";
       echo "<input type=submit value=\"save\"></td>\n";
       echo "<td align=center valign=middle id=\"tbhel\">\n";
       echo "<input type=reset value=\"reset\"></td>\n";
       echo "</form>\n";

       if (!($z[5] & 0x04))
       {
         echo "<td align=center valign=middle height=20 id=\"tbhel\">\n";
         echo "<form name=\"delentry\"";
         echo " action=\"changeconf.php\" target=\"status\" method=post>\n";
         echo "<input type=hidden name=\"action\" value=\"ldel\">\n";
         echo "<input type=hidden name=\"list\" value=\"$list\">\n";
         echo "<input type=hidden name=\"key\" value=\"$z[1]\">\n";
         echo "<input type=hidden name=\"value\" value=\"$z[3]\">\n";
         echo "<input type=submit value=\"Delete\"></td></form>\n";
       }
       else
         echo "<td align=center valign=middle height=20 id=\"tbhel\">&nbsp;</td>\n";

       echo "</tr><tr bgcolor=#ffffff><td height=2></td></tr>\n";
     }
     else
     {
       echo "<tr><td id=\"tbhel\"><tt><font id=\"dark\">\n";
       echo "<a href=\"describekey.php?key=$z[1]\" target=\"status\">$z[1]\n";
       echo "</a></font></tt></td>\n";
       echo "<td height=20 id=\"tbhel\"><tt><font id=\"dark\">$z[3]</font></tt></td>\n";
       echo "<td height=20 id=\"tbhel\"><tt><font id=\"dark\">&nbsp;</font></tt></td>\n";
       echo "<td height=20 id=\"tbhel\">&nbsp;</td><td height=20 id=\"tbhel\">&nbsp;\n";
       echo "</td></tr><tr bgcolor=#ffffff><td height=2></td></tr>\n";
     }
     $z = next($main);
   }

   // end table
   echo "</table>\n";

   // add key
   echo "<form name=\"add\" action=\"changeconf.php\" target=\"status\" method=post ";
   echo "onSubmit=\"return chkchosen()\">\n";
   echo "<input type=hidden name=\"action\" value=\"ladd\">\n";
   echo "<input type=hidden name=\"list\" value=\"$list\">\n";
   echo "<select name=\"key\" size=1 onChange=\"doJump(this)\">\n";
   echo "<option value=\"oops\">choose key to add\n";

   reset($keyz);
   $k = current($keyz);
   while ($k)
   {
     if ((($k[7] & 0x02) || (!strcmp($k[2], "true"))) && !($k[7] & 0x20) && !($k[7] & 0x10))
       echo "<option value=\"$k[1]\">$k[1]\n";
     $k = next($keyz);
   }

   echo "</select>\n";
   echo "<input type=text name=\"value\" size=30>\n";
   echo "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n";
   echo "<input type=submit value=\"Add\"> &nbsp;&nbsp;&nbsp; \n";
   echo "<input type=reset value=\"Cancel\">";
   echo "</form>\n";

   endcon ($fp);
 }
?>

</body>
</html>
