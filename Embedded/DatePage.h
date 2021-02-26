const String DATEPAGEHTML = R""""(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Air Quality Data</title>
    <link rel="stylesheet" href="https://code.jquery.com/ui/1.12.1/themes/base/jquery-ui.css">
    <script src="https://code.jquery.com/jquery-1.12.4.js"></script>
    <script src="https://code.jquery.com/ui/1.12.1/jquery-ui.js"></script>
    <script>
    $(function() {
      $( "#datepicker" ).datepicker();
    } );
    </script>
</head>
<body>

<p>Date: <input type="text" id="datepicker" onChange="load_data(this.value);"></p>
<div id="chartDiv" style="width:100%; height:600px; margin:0 auto;"></div>

</body>

<script src="https://code.jscharting.com/2.9.0/jscharting.js"></script>
<script>

function load_data(date) {

  if (date == "") {
      return;
  }

  const HOST = "192.168.50.41";
  const PORT = 9000;

  console.log( "Going to update data; date: " + date);

  let currentTime = new Date(date);
  currentTime.setHours(0,0,0,0);

  let startDateSeconds = Math.floor(currentTime.getTime() / 1000);
  let request = "http://" + HOST + ":" + PORT + "/data?DATE=" + startDateSeconds + "&AGGREGATION=900";

  console.log("startDateSeconds: " + startDateSeconds);
  console.log(request);

  fetch(request)
    .then(data => { return data.json()})
    .then(res => {
        let minData = [], maxData = [], average = [];
        for (var i = 0; i < res.length; i++) {
            var obj = res[i];
            var fullDate = new Date(obj[0] * 1000);
            // Hours part from the timestamp
            var hours = fullDate.getHours();
            // Minutes part from the timestamp
            var minutes = "0" + fullDate.getMinutes();
            // Seconds part from the timestamp
            var seconds = "0" + fullDate.getSeconds();

            // Will display time in 10:30:23 format
            var formattedTime = hours + ':' + minutes.substr(-2) + ':' + seconds.substr(-2);

            minData.push( {x: fullDate, y: obj[1] } );
            maxData.push( {x: fullDate, y: obj[2] } );
            average.push( {x: fullDate, y: obj[3] } );
        }
        return [
            { name: 'Min', points: minData },
            { name: 'Max', points: maxData },
            { name: 'Average', points: average }
        ] })
    .then(series => {
        renderChart(series);
     });
}

function renderChart(series) {
	JSC.Chart('chartDiv', {
    legend_defaultEntry_value: '',
    yAxis: { formatString: 'n', label_text: 'PM10' },
    xAxis: { scale_type: 'time' },
    defaultPoint_tooltip: function(point) {
        let d = new Date(point.options('x'))
        var hours = d.getHours();
        var minutes = "0" + d.getMinutes();
        let value = point.options('y')
        return '%seriesName' + '<br>PM10: ' + value + '<br>Time: ' + hours + ":" + minutes.substr(-2);
    },
		series: series
	});
}

</script>
</html>
)"""";
