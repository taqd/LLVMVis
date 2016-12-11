<?php
error_reporting(E_ALL & ~E_NOTICE);
require_once 'markdown/Markdown.inc.php';
use \Michelf\Markdown;

$dataset    = 'default';
$dataset_qs = ''; //Query string
$epoch = 'epoch0';

//Get the epoch number (run number) from either the 'count' file, or
//the url, if neither are found, use epoch0
if (isset($_GET['epoch'])) {
    if (!preg_match('@[^a-z0-9-_ ]@i', $_GET['epoch'])) {
        $epoch      = $_GET['epoch'];
    }
} else {
    $epochFile = fopen("data/count.txt", "r") or die("Unable to open file!");
    $epochTmp = intval(fgets($epochFile));
    $epoch = "epoch$epochTmp"; 
    fclose($epochFile);
}

//Get the dataset name from the url
if (isset($_GET['dataset'])) {
    if (!preg_match('@[^a-z0-9-_ ]@i', $_GET['dataset'])) {
        if (is_dir('data/' . $epoch . '/' . $_GET['dataset'])) {
            $dataset    = $_GET['dataset'];
            $dataset_qs = "?dataset=$dataset";

        }
    }
}

//Load in the data used in the side bar (written in markdown)
function get_html_docs($obj) {
    global $config, $data, $dataset, $errors, $epoch;

    $name = str_replace('/', '_', $obj['name']);
    $filename = "data/$epoch/$dataset/$name.mkdn";

    $name = str_replace('_', '\_', $obj['name']);
    $type = $obj['type'];
    if ($config['types'][$type]) {
        $type = $config['types'][$type]['short'];
    }
    
    $group = $obj['group'];
    if ($config['groups'][$group]) {
        $type = $config['types'][$type]['short'];
    }
    
    //Beginning of markdown
    $markdown = "##<center>$name</center>\n";
    $markdown .= "<b>Type:</b> $type<br />";
    $markdown .= "<b>Group:</b> $group<br />\n\n";

    if ($obj) {
        $markdown .= get_depends_markdown('<b>Depends on:</b> ',     $obj['depends'])."<br />";
        $markdown .= get_depends_markdown('<b>Depended on by:</b> ', $obj['dependedOnBy'])."<br />\n\n";
        $markdown .= get_epochs($obj['name'])."<br />\n\n";                
    }

    //Read in the data from the .mkdn file
    if (file_exists($filename)) {
        $markdown .= file_get_contents($filename);
    } else {
        $markdown .= "<div class=\"alert alert-warning\">No documentation for this object</div>";
    }

    // Use {{object_id}} to link to an object in the markdown
    $arr      = explode('{{', $markdown);
    $markdown = $arr[0];
    for ($i = 1; $i < count($arr); $i++) {
        $pieces    = explode('}}', $arr[$i], 2);
        $name      = $pieces[0];
        $id_string = get_id_string($name);
        $name_esc  = str_replace('_', '\_', $name);
        $class     = 'select-object';
        if (!isset($data[$name])) {
            $class .= ' missing';
            $errors[] = "Object \"$obj[name]\" links to unrecognized object \"$name\"";
        }
        $markdown .= "<a href=\"#$id_string\" class=\"$class\" data-name=\"$name\">$name_esc</a>";
        $markdown .= $pieces[1];
    }

    $html = Markdown::defaultTransform($markdown);
    
    // IE can't handle <pre><code> (it eats all the line breaks)
    $html = str_replace('<pre><code>'  , '<pre>' , $html);
    $html = str_replace('</code></pre>', '</pre>', $html);
    return $html;
}


//Get the list of epochs available from the data/ directory
function get_epochs() {
    global $dataset, $epoch;

    $directory = "data/epoch*";
    $phpfiles = glob($directory);

    //Create [First | Prev | x | Next | Last] navigation bar    
    $first = basename(reset($phpfiles));
    $prev = intval(substr($epoch,5))-1;
    $next = intval(substr($epoch,5))+1;
    $markdown = "<b>History:</b> [";    
    $markdown .= "<a href=graph.php?dataset=".$dataset."&epoch=".$first.">First</a> | ";
    $markdown .= "<a href=graph.php?dataset=".$dataset."&epoch=epoch".$prev .">Prev</a> | ";
    $markdown .= " @".substr($epoch,5)." | ";
    $markdown .= "<a href=graph.php?dataset=".$dataset."&epoch=epoch".$next.">Next</a> | ";
    $markdown .= "<a href=graph.php?dataset=".$dataset.">Newest</a>";    
    $markdown .= " ] : [";

    //Create list of epochs
    foreach ($phpfiles as $phpfile) {
        $base = substr(basename($phpfile),5);
        $markdown .= "<a href=graph.php?dataset=".$dataset."&epoch=".basename($phpfile).">".$base."</a> ";
    }
    $markdown .= "]";    
    return $markdown;
}

//Create a list of all the dependencies. Make them clickable.
function get_depends_markdown($header, $arr) {
    $markdown = "$header";
    if (count($arr)) {
        $markdown .= "";
        foreach ($arr as $name) {
            $markdown .= "{{" . $name . "}}, ";
        }
        $markdown .= "";
    } else {
        $markdown .= "None";
    }
    return $markdown;
}

function get_id_string($name) {
    return 'obj-' . preg_replace('@[^a-z0-9]+@i', '-', $name);
}

//Read in the config.json file
function read_config() {
    global $config, $dataset, $dataset_qs, $epoch;

    $config = json_decode(file_get_contents("data/$epoch/$dataset/config.json" ), true);

    //Not entirely sure what this is for, but I gather it is the
    //connection between the php and javascript
    $config['jsonUrl'] = "json.php$dataset_qs&epoch=$epoch"; 
}

//Read in the data file from objects.json
function read_data() {
    global $config, $data, $dataset, $errors, $epoch;

    if (!$config) read_config();

    $json   = json_decode(file_get_contents("data/$epoch/$dataset/objects.json"), true);    
    $data   = array();
    $errors = array();

    foreach ($json as $obj) {
        $data[$obj['name']] = $obj;
    }

    foreach ($data as &$obj) {
        $obj['dependedOnBy'] = array();
    }
    unset($obj);
    foreach ($data as &$obj) {
        foreach ($obj['depends'] as $name) {
            if ($data[$name]) {
                $data[$name]['dependedOnBy'][] = $obj['name'];
            } else {
                $errors[] = "Unrecognized dependency: '$obj[name]' depends on '$name'";
            }
        }
    }
    unset($obj);
    foreach ($data as &$obj) {
                $obj['docs'] = get_html_docs($obj);
    }
    unset($obj);
}
?>
