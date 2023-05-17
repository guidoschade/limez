<html>
<head><title>Error Occured</title>
<link rel=stylesheet type="text/css" href="format.css">
</head>
<body text='#000000' bgcolor='#ffffff'>

<table width=640 align=center border="0" cellpadding="0" cellspacing="0" bgcolor=#acaccc>
<tr><td width=100% height=50 bgcolor=#ffffff>&nbsp;</td></tr>

<tr><td width=100% valign=top><font face="Arial,Helvetica" size="3"></td></tr>

<tr><td width=100% valign=top>&nbsp;</td></tr>

<tr><td class="abstand" width=100% valign=top><font face="Arial,Helvetica" size="3">
    <p>&nbsp;<br><dir><b>Error</b><p><dir>
    Error occured while connecting to limez server
    </dir>
    <p><font id="error"><b>Limez Server</b></font>
    <p>
    <dir>
    <?php
      include ("limez.ini");
      include ("limez.lib");
      echo "Host: $limez_host<br>Port: $limez_port<br>";
    ?>
    </dir>
    <p>
    <font id="error"><b>Error Message</b></font>
    <p><dir>
    <?php
      echo $error;
    ?>
    </dir>
    <p><font id="error"><b>Possible reasons</b></font>
    <p>
    <ul type="."><font id="error">
      <li>wrong IP address or port number in 'limez.ini'
      <li>Limez list server is currently not running (or version < 1.0)
      <li>the port is used by other mail transfer agents (e.g. sendmail)
      <li>wrong password of Limez Admin in 'limez.ini'
      <li>wrong IP address set in list server main config</font>
    </ul>
    <p><a href="index.html"<font id="error"><b>Try again</b></font></a>
    <p>&nbsp;<p>
    </dir>
    </td>
</tr>
</table>







</body>
</html>



