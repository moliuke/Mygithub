<html>
<!- Copyright (c) Go Ahead Software Inc., 2000-2010. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<TITLE>功能参数配置</TITLE>
<link rel="stylesheet" type="text/css" href="w3c.css">
<script>
var int=self.setInterval("clock()",1000)
function clock(){
var localtime;
localtime = new Date();
document.getElementById("year").value=localtime.getFullYear();
document.getElementById("mon").value=localtime.getMonth()+1;
document.getElementById("day").value=localtime.getDate();
document.getElementById("hour").value=localtime.getHours();
document.getElementById("min").value=localtime.getMinutes();
document.getElementById("sec").value=localtime.getSeconds();
}
setTimeout("clock()",100);
</script>


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

<form action="/goform/formTime" method="post">
<input id="year" name="year" size="4" style="width:30px;border: none;"/>年
<input id="mon" name="mon" size="2" style="width:15px;border: none;"/>月
<input id="day" name="day" size="2" style="width:15px;border: none;"/>日&nbsp
<input id="hour" name="hour" size="2" style="width:15px;border: none;"/>时
<input id="min" name="min" size="2" style="width:15px;border: none;"/>分
<input id="sec" name="sec" size="2" style="width:15px;border: none;"/>秒&nbsp&nbsp
<button type="submit">时间同步</button>
</form>
<h3>获取播放内容的方式</h3>

<select onchange="onChangeContent()" id="selectContent">
  <option value="1">1</option>
  <option value ="2">2</option>
  <option value ="3">3</option>
</select>
<h3>检测TXRX模式</h3>
<select onchange="onChangeCheck()" id="selectCheck">
  <option value ="ON">ON</option>
  <option value="OFF">OFF</option>
</select>
<h3>定时复位TXRX</h3>
<select onchange="onChangeReset()" id="selectReset">
  <option value ="ON">ON</option>
  <option value="OFF">OFF</option>
</select>
<h3>像素点检测</h3>
<select onchange="onChangePix()" id="selectPix">
  <option value="OFF">OFF</option>
  <option value ="ON">ON</option>
</select>
<h3>ping网关</h3>
<select onchange="onChangePing()" id="selectPing">
 <option value ="ON">ON</option> 
 <option value="OFF">OFF</option>
</select>
<br><br>

</div>
</div>

<form action="/goform/formCheck" method="POST">
<fieldset style="border-style:solid;border-width:1.5pt; border-color:blue" cellspacing="10" cellpadding="10" align="center">
<legend>当前配置信息</legend>
<td width="70" height="50">获取播放内容的方式：</td><input style="width:60px;border: none;" readonly=“readonly” type="text" name="mode" value="<%GetCheck('mode'); %>" size="16" maxlength="3"> <br>
<td width="70" height="50">检 测 TXRX 模 式：</td><input style="width:60px;border: none;" readonly=“readonly” type="text" name="check" value="<%GetCheck('check'); %>" size="16" maxlength="3"> <br>	
<td width="70" height="50">定 时 复 位 TXRX：</td><input style="width:60px;border: none;" readonly=“readonly” type="text" name="reset" value="<%GetCheck('reset'); %>" size="16" maxlength="3"><br>
<td width="70" height="50">像 素 点 检 测：</td><input style="width:60px;border: none;" readonly=“readonly” type="text" name="pix" value="<%GetCheck('pix'); %>" size="16" maxlength="3"><br>
<td width="70" height="50">ping 网 关：</td><input style="width:60px;border: none;" readonly=“readonly” type="text" name="ping" value="<%GetCheck('ping'); %>" size="16" maxlength="3"><br>
</fieldset>

<button type="submit">提交</button>
<button type="reset" value="重置">重置</button>
</form>

<form action="/goform/formBright" method="POST">
<fieldset style="border-style:solid;border-width:1.5pt; border-color:blue" cellspacing="10" cellpadding="10" align="center">
<legend>光敏异常</legend>
<td width="70" height="50">白天亮度值：</td><input id="daytime" type="text" placeholder="输入1-31数值" name="daytime" value="<%GetBright('daytime'); %>" size="16" maxlength="2"> <br>
<td width="70" height="50">黑夜亮度值：</td><input id="night" type="text" placeholder="输入1-31数值" name="night" value="<%GetBright('night'); %>" size="16" maxlength="2"><br>
</fieldset>
<input type="button" onclick="numcheck()" value="提交" style="background-color: teal;color: white;border-radius: 4px;padding: 2px 15px 2px 15px;"></input>
<input type="submit" id="btnSumbit" hidden="hidden"></input>
<button type="reset" value="重置">重置</button>
</form>

<form action="/goform/formBrightRange" method="POST">
<fieldset style="border-style:solid;border-width:1.5pt; border-color:blue" cellspacing="10" cellpadding="10" align="center">
<legend>亮度等级调整</legend>
<td width="70" height="50">最高亮度等级：</td><input id="MaxBright" type="text" placeholder="输入1-31数值" name="MaxBright" value="<%GetBrightRange('MaxBright'); %>" size="16" maxlength="2"> <br>
<td width="70" height="50">最低亮度等级：</td><input id="MinBright" type="text" placeholder="输入1-31数值" name="MinBright" value="<%GetBrightRange('MinBright'); %>" size="16" maxlength="2"> <br>
</fieldset>
<input type="button" onclick="brightcheck()" value="提交" style="background-color: teal;color: white;border-radius: 4px;padding: 2px 15px 2px 15px;"></input>
<input type="submit" id="BrightSumbit" hidden="hidden"></input>
<button type="reset" value="重置">重置</button>
</form>


<form action="/goform/formReboot" method="POST">

<input type="text" name="name" hidden="hidden"></input>
<button type="submit" >设备复位</button>
</form>


<div id="footer">
<hr/>
Page refresh at <!--#t--> @ seewor.com
</div>
<script language="JavaScript" type="text/javascript" src="fun.js"></script>
</body>
</html>