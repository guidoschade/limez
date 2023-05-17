<html>
<head><title>New List</title>
<link rel=stylesheet type="text/css" href="format.css">
<script language="JavaScript">
<!--
function chkdb()
{
  if (document.newlist.lname.value == "")
  {
    alert ("Please enter a list name");
    document.newlist.lname.focus();
    return false;
  }
  if (document.newlist.type[1].checked == true)
  {
    if (document.newlist.fname.value == "")
    {
      alert ("Please enter location of config file");
      document.newlist.fname.focus();
      return false;
    }
  }
}
function chkfile()
{
  if (document.newlist.lname.value == "")
  {
    alert ("Please enter a list name");
    document.newlist.lname.focus();
    return false;
  }
  if (document.newlist.type.value == "list")
  {
    if (document.newlist.fname.value == "")
    {
      alert ("Please enter location of config file for the new list");
      document.newlist.fname.focus();
      return false;
    }
  }
}
//-->
</script>
</head>
<body text='#000000' bgcolor='#ffffff'>

<table align=center width=75% border=0 bgcolor=#eeeeee cellpadding=0 cellspacing=0>
<tr><td id="whi" colspan=4 height=70>&nbsp;</td></tr>
<tr><td colspan=4 height=50>&nbsp;</td></tr>
<tr><td width=10%>&nbsp;</td>

<?php

include ("limez.ini");
include ("limez.lib");

if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "LSHW config limez") == 1)
{
  // add all main config keys to hash keyz
  while(1)
  {
    $x = fgets ($fp,128);
    if (strncmp($x, $end, 4))
      break;

    $keyz[] = explode("\"", substr($x,4));
  }

  endcon ($fp);

  $count = count($keyz);
  $db = 0;
  for ($y = 0; $y<$count; $y++)
  {
    if (!strcmp($keyz[$y][1], "server_config"))
    {
      if (!strcmp($keyz[$y][3], "database"))
        $db = 1;
    }
  }

  echo "<form name=\"newlist\" action=\"newlist.php\" target=\"status\" method=post ";

  if ($db)
    echo "onSubmit=\"return chkdb()\">\n";
  else
    echo "onSubmit=\"return chkfile()\">\n";

  echo "<td><font id=\"hint\">New List Name:</td><td>\n";
  echo "<input type=text name=\"lname\" value=\"\" size=20></td>\n";
  echo "<td width=10%>&nbsp;</td></tr><tr>\n";

  if ($db)
  {
    echo "<td>&nbsp;</td><td valign=top><font id=\"hint\">Type:</td><td>\n";
    echo "<input type=radio name=\"type\" value=\"dblist\" checked><font id=\"hint\">Database<br>\n";
    echo "<input type=radio name=\"type\" value=\"list\"><font id=\"hint\">File&nbsp;&nbsp;&nbsp;\n";
    echo "<input type=text name=\"fname\" value=\"\" size=15 maxlength=64></td><td>&nbsp;</td></tr>\n";
  }
  else
  {
    echo "<td>&nbsp;</td><td valign=middle><font id=\"hint\">Location of Config File:</td><td>\n";
    echo "<input type=hidden name=\"type\" value=\"list\">\n";
    echo "<input type=text name=\"fname\" value=\"\" size=20 maxlength=64></td><td>&nbsp;</td></tr>\n";
  }
}

?>

<tr>
<td>&nbsp;</td>
<td>&nbsp;</td>
<td><input type=submit value="OK">
<input type=reset value="Cancel"></td>
<td>&nbsp;</td>
</tr>
</form>
<tr>
<td colspan=4>&nbsp;</td>
</tr>
<tr>
<td>&nbsp;</td>
<td colspan=2><p align=justify><font id="hint">

<?php

  if ($db)
  {
    echo "You can choose between storing mailing list configuration in database or in the given file. \n";
    echo "If you use the database config Limez uses the 'list_name' as default table for storing users. \n";
    echo "Suppose your list name contains special characters (like '.' or '-'), please add an entry \n";
    echo "'list_usertable_name'  \n";
    echo "in the list config to prevent database error messages. \n";
    echo "Please check the values of 'list_user_source' and 'list_sender_source' before you \n";
    echo "request USER or SENDER of this new list.\n";
  }
  else
  {
    echo "Limez does not support databases in its current configuration. You can only \n";
    echo "create mailing lists stored in files. \n";
    echo "Either you did not start Limez with 'server_config' = 'database' or \n";
    echo "you did not even compile it with database option. Please refer to the file INSTALL.\n";
    echo "Please change the values of 'list_user_source' and 'list_sender_source' before you request USER \n";
    echo "or SENDER of this new list.\n";
  }

?>

</font></td><td>&nbsp;</td></tr>
<tr><td colspan=4 height=50>&nbsp;</td></tr>
</table>

</body>
</html>