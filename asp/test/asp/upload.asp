<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>File Upload Demo</title>
<!-- date  5-Mar-2020 -->
</head>
<body>
<h3>工控机升级文件上传</h3>

<table>
<% GetFileList("/home/LEDscr/config/cls.conf"); %>
</table>

<form action="/goform/formUploadFileTest" method="POST" enctype="multipart/form-data">
   Select file: <input type="file" name="binary"><br>
  <input type="submit" value="发送">
  <input type="reset" value="重置">
</form>
</body>
</html>
