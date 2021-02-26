<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<TITLE>升级</TITLE>
<link rel="stylesheet" type="text/css" href="w3c.css">
<!-- date  5-Mar-2020 -->
</head>
<body>

<div id="header">
<img src="swr.jpg" align ="left" width="160" height = "70">
<h2>工控机配置</h2>
</div>
<div id="nav">
<nobr><A HREF="ip.asp">IP配置</A>
<A HREF="protocol.asp">协议选择</A>
<A HREF="config.asp">功能选择</A>
<A HREF="upload.asp">工控机升级</A></nobr>
</div>
<div id="section">
<div id="myborder">

<p>配置文件下载</p><a href="home/LEDscr/config/cls.conf" download="cls.conf">点击下载</a> 
<br><br><br>
<p>运行日志下载</p><a href="home/LEDscr/sys/daemon.log" download="daemon.log">点击下载</a> 
<br><br><br>
<form action="/goform/formUploadFileTest" method="POST" enctype="multipart/form-data">

<h4>升级文件发送</h4>
  <input type="file" name="binary"><br>
  <br>
  <button type="submit" value="发送">发送</button>
  <button type="reset" value="重置">重置</button>
</form>
</div>
</div>
<div id="footer">
<hr/>
Page refresh at <!--#t--> @ seewor.com
</div>
</body>
</html>
