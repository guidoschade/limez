<html>
<head><title>LIMEZ ADMIN</title>
<link rel=stylesheet type="text/css" href="format.css">
<script language="JavaScript">
<!--
function chkinput()
{
  if (document.newlist.lname.value == "")
  {
    alert ("Please enter a name");
    document.newlist.lname.focus();
    return false;
  }
}
function goservconf()
{
  parent.frames[1].location.href = "config.php?list=limez";
  parent.frames[3].location.href = "status.php";
}
function goreload()
{
  parent.frames[1].location.href = "blank.html";
  parent.frames[3].location.href = "reload.php";
}
function goconf()
{
  var list = document.forms[0].elements[0].options[document.forms[0].elements[0].options.selectedIndex].value; 
  if (!list) alert ("Please choose a list")
  else
  {
    var x = "config.php?list=" + list;
    parent.frames[1].location.href = x;
    parent.frames[3].location.href = "status.php";
  }
}
function gosender()
{
  var list = document.forms[0].elements[0].options[document.forms[0].elements[0].options.selectedIndex].value; 
  if (!list) alert ("Please choose a list")
  else
  {
    var x = "editsender.php?list=" + list;
    parent.frames[1].location.href = x;
    parent.frames[3].location.href = "status.php";
  }
}
function gouser()
{
  var list = document.forms[0].elements[0].options[document.forms[0].elements[0].options.selectedIndex].value; 
  if (!list) alert ("Please choose a list")
  else
  {
    var x = "edituser.php?num=1&list=" + list;
    parent.frames[1].location.href = x;
    parent.frames[3].location.href = "status.php";
  }
}
function del()
{
  var list = document.forms[0].elements[0].options[document.forms[0].elements[0].options.selectedIndex].value;
  if (!list) alert ("Please choose a list")
  else
  {
    check = confirm("Do you really want to delete list '" + list + "' ?");
    if(check == false)
    {
      parent.frames[1].location.href = "blank.html";
      parent.frames[3].location.href = "status.php";
    }  
    else
    {
      var x = "dellist.php?list=" + list;
      parent.frames[1].location.href = "blank.html"; 
      parent.frames[3].location.href = x;
    }
  }
}
function newlistcheck()
{
  parent.frames[1].location.href = "newlistcheck.php";
  parent.frames[3].location.href = "status.php";
}
function doJump()
{
  parent.frames[1].location.href = "right.php";
  parent.frames[3].location.href = "status.php";
}
//-->
</script>
</head>
<body text='#000000' bgcolor='#aaaaaa'>

<table align=center width=90% cellpadding=8>
<tr><td id="tbdkl" align=center><font face="Arial, Helvetica, sans-serif" id="big">
<b><a href="http://www.limez.net" target="new"><font color=#ffffff>Limez Admin</font></a></b><br>v1.0</font></td></tr>
</table>
<p><center>

<?php
 include ("limez.ini");
 include ("limez.lib");

 if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "LSHW lists") == 1)
 {
   // show config and reload
   echo "<table align=center width=75% border=0><tr><td>";
   echo "<a href=\"javascript:goservconf()\">\n";
   echo "<img src=\"images/g_serverconfig.gif\" border=0></a></td></tr>\n";
   echo "<tr><td><a href=\"javascript:goreload()\">\n";
   echo "<img src=\"images/g_reload.gif\" border=0></a></td></tr></table>\n";

   // show existing mailing lists
   while(1)
   {
     $x = fgets ($fp,128);
     if (strncmp($x, $end, 4))
        break;
     $y = substr($x,4,-2);
     $bla[]=$y;
   }
   $c = count($bla);
   if ($c < 4)
     $d = $c;
   else
     $d = 3; 

   // show mailinglists
   echo "<hr>";
   echo "<img src=\"images/g_lists.gif\" border=0>";
   for ($s = 1; $s <= strlen($c); $s++) 
     echo "<img src=\"images/".substr($c,$s-1,1).".gif\" border=0>\n";
   if ($c > 0)
   {
     echo "<form name=\"mail\">\n";
     echo "<select name=\"lists\" size=1 onChange=\"doJump()\">\n";

     sort($bla);
     reset($bla);
     $z = current($bla);
     while ($z)
     {
       echo "<option value=\"$z\">$z&nbsp;&nbsp;&nbsp;&nbsp;\n";
       $z = next($bla);
     }
     echo "</select></form>\n";

     // Edit mailing list config, sender, user
     // echo "<p>View / Edit<br>\n";

     echo "<table align=center width=75% border=0><tr><td colspan=2>";
     echo "<a href=\"javascript:goconf()\">\n";
     echo "<img src=\"images/g_config.gif\" border=0></a></td></tr>\n";
     echo "<tr><td><a href=\"javascript:gosender()\">\n";
     echo "<img src=\"images/g_sender.gif\" border=0></a></td>\n";
     echo "<td><a href=\"javascript:gouser()\">\n";
     echo "<img src=\"images/g_user.gif\" border=0></a></td></tr>\n";

     // delete list
     echo "<tr><td colspan=2><a href=\"javascript:del()\">\n";
     echo "<img src=\"images/g_delete.gif\" border=0></a></td></tr></table>\n";
   }

   // add new list
   echo "<a href=\"javascript:newlistcheck()\">";
   echo "<p align=center><img src=\"images/g_create.gif\" border=0></a>";
   echo "<hr>";

   // show config syntax
   echo "<a href=\"showkeys.php\" target=\"right\">";
   echo "<img src=\"images/g_syntax.gif\" border=0></a>";
   echo "</center>";

   endcon ($fp);
 }
?>

</body>
</html>
