   function check(){
      var name=document.getElementById("name").value;
   var pass=document.getElementById("pass").value;
   if(name=="admin" && pass=="admin"){
   window.location.href="ip.asp";
 
   }else{
   alert("用户名或密码错误");   
   }
   
   }
   
 	function numcheck()
	{
		var daytime = document.getElementById("daytime").value;
		var night = document.getElementById("night").value;
		if(daytime > 0 && daytime < 32 && night > 0 && night < 32)
		{
			if(parseInt(night) >= parseInt(daytime))
			{
				alert("白天亮度值要大于夜晚亮度值");
				return false;
			}
			var val = document.getElementById("btnSumbit");
			val.click();
		}	
		else
		{
			alert("请输入范围在1-31的值");
			return false;
			
		}
		
	}
	function brightcheck()
	{
		var max = document.getElementById("MaxBright").value;
		var min = document.getElementById("MinBright").value;
		if(max > 0 && max < 32 && min > 0 && min < 32 )
		{
			if(parseInt(min) >= parseInt(max))
			{
				alert("最大亮度等级要大于最小亮度等级");
				return false;
			}
			var val = document.getElementById("BrightSumbit");
			val.click();
		}	
		else
		{
			alert("请输入范围在1-31的值");
			return false;
			
		}
	}	


	function onChangeContent()
	{
		var option = document.getElementById("selectContent").value;
		console.log(option);
		document.getElementsByName("mode")[0].value = option;
	}
	function onChangeCheck()
	{
		var option = document.getElementById("selectCheck").value;
		console.log(option);
		document.getElementsByName("check")[0].value = option;
	}	
	function onChangeReset()
	{
		var option = document.getElementById("selectReset").value;
		console.log(option);
		document.getElementsByName("reset")[0].value = option;
	}
	function onChangePix()
	{
		var option = document.getElementById("selectPix").value;
		console.log(option);
		document.getElementsByName("pix")[0].value = option;
	}	
	function onChangePing()
	{
		var option = document.getElementById("selectPing").value;
		console.log(option);
		document.getElementsByName("ping")[0].value = option;
	}		