/*
*  ライブラリのdraw_base/spriteを出力する
*
*  ・movieフォルダ内のMovieClipが対象
*   ・layer名が、col_/none以外で始まるものが対象
*     ・colはcollisionレイヤー。noneはガイドなどに使えるようにするため
*  ・sprite用MovieClipはspriteフォルダにいれておく
*   ・spriteIDとして使うので、movie側と名前被りに注意
*   ・spriteシートを作成し、spriteタグを生成する
*  ・pngフォルダに、使用する元の画像データをいれておく */
fl.outputPanel.clear(); 

var doc 		= fl.getDocumentDOM();
var name 		= doc.name.replace(".fla","");
var namePath 	= doc.pathURI.replace(".fla","");
var xmlPath 	= namePath + "_draw_base.xml";

// データ定義
function pos(x,y)
{
	this.x = x;
	this.y = y;
}

function rect(left, top, right, bottom)
{
	this.left	 = left;
	this.top	 = top;
	this.right  = right;
	this.bottom = bottom;
}

function draw()
{
	this.x	 	 = 0;
	this.y	 	 = 0;
	this.width  = 1.0;
	this.height = 1.0;
	this.angle  = 0;
	// 色0～255
	this.a  	 = 255;
	this.r  	 = 255;
	this.g  	 = 255;
	this.b  	 = 255;
}

// kind
DRAW_BASE	= 0;
SPRITE		= 1;
KEYFRAME	= 2;
TWEEN		= 3;
TIMELINE	= 4;
MOVIE_CLIP	= 5;
AUDIO     	= 6;

function base()
{
	this.kind	= DRAW_BASE;
	this.id	= "";
	this.pivot = new pos(0,0);
	this.draw  = new draw();
	this.child = new Array();
}

function sprite()
{
	this.base 		= new base();
	this.base.kind = SPRITE;
 
	this.tex  = "";
	this.rect = new rect(0,0,0,0);
}

function keyframe()
{
	this.base 		= new base();
	this.base.kind = KEYFRAME;
	 
	this.next   = 1;
}

function tweenframe()
{
	this.base		= new base();
	this.base.kind = TWEEN;
 
	this.draw_after= new draw();
	this.easing	= 0.0;
 
	this.next   = 1;
}

function movieclip()
{
	this.base 		= new base();
	this.base.kind = MOVIE_CLIP;
}

// sprite/state/png分解
var lib = doc.library;
var states  = new Array();

var sse				= new SpriteSheetExporter();
sse.format			= "RGBA8888";
sse.layoutFormat	= "JSON-Array";
sse.autoSize		= true;
sse.maxSheetWidth	= 2048;
sse.maxSheetHeight	= 2048;
sse.allowRotate 	= false;
sse.allowTrimming 	= false;


for(i in lib.items)
{
	var it = lib.items[i];
	if(it.itemType=="movie clip")
	{
		if(it.name.search("^movie/")!=-1)
		{
			states.push(it);
		}
	}
	else if(it.itemType=="bitmap")
	{
		if(it.name.search("^png/")!=-1)
		{
			sse.addBitmap(it);
		}
	}
}

/////////////////////////
// sprite関係
var sprite_bases = new Array();

var ssePath = namePath + "_sheet.png";
var sseName = name + "_sheet";

// SpriteSheetExporterを使ってspriteシートを作る
sse.exportSpriteSheet(namePath + "_sheet", {format:"png", bitDepth:32, backgroundColor:"#00000000"});

// evalで出力したJSONを読み直す
var sheetJson = eval( "(" + FLfile.read(namePath + "_sheet.json") + ")");

// sprite情報を一端貯める
var sseFrames = sheetJson['frames'];
for(s in sseFrames)
{
	var spInfo = sseFrames[s];
	 
	var spBase = new sprite();
	spBase.tex = sseName;
	spBase.rect.left	= spInfo['frame']['x'];
	spBase.rect.top		= spInfo['frame']['y'];
	spBase.rect.right 	= spBase.rect.left + spInfo['frame']['w'];
	spBase.rect.bottom	= spBase.rect.top  + spInfo['frame']['h'];

	var spID  = spInfo['filename'].replace(".png","");
	//fl.trace(spID);
	var spSym = "sprite/" + spID;
	if(lib.editItem(spSym))
	{
		spBase.base.id = spID;
		var e = doc.getTimeline().layers[0].frames[0].elements[0];
		spBase.base.pivot.x = -e.x;
		spBase.base.pivot.y = -e.y;
	}
	else
	{
		fl.trace(spSym + "が見つかりませんでした。");
	}

	sprite_bases.push(spBase);
}

////////////////////////
// draw_base(MovieClip)関係
function parseFromElementToDraw(e, draw)
{
draw.x 		= Math.round(e.x);
draw.y 		= Math.round(e.y);
draw.width 	= e.scaleX;
draw.height = e.scaleY;
draw.angle 	= e.rotation;
 
// カラー情報はinstanceのみからしか取れないらしい？
if(e.elementType == "instance")
{// プロパティのカラー効果で設定
	if(e.colorMode=="alpha")
	{// スタイル:アルファ
		draw.a = e.colorAlphaPercent * 255 / 100;
	}
	else if(e.colorMode=="tint")
	{// スタイル:着色
		draw.a = e.tintPercent * 255 / 100;
		
		fl.trace(e.tintColor);
		
		var r = e.tintColor.substr(1,2);
		draw.r = parseInt(r, 16);
		var g = e.tintColor.substr(2,2);
		draw.g = parseInt(g, 16);
		var b = e.tintColor.substr(5,2);
		draw.b = parseInt(b, 16);
	}
}
}

// movieclipなどのnameからフォルダパスを取り除く
function getDrawBaseID(id)
{
	if(id.search("^movie/")!=-1)
	{
		return id.replace("movie/","");
	}
	else if(id.search("^sprite/")!=-1)
	{
		return id.replace("sprite/","");
	}
	else if(id.search("^png/")!=-1)
	{
		return id.replace("png/","");
	}

	return id;
}

function parseFromFrame(frame, key)
{	
	for(i in frame.elements)
	{
		var b = new base();
		var e = frame.elements[i];
		parseFromElementToDraw(e, b.draw);

		b.id = getDrawBaseID(e.libraryItem.name);
		key.base.child.push(b);
}
}

function parseFromLayer(layer)
{
	var tl = new base();
	tl.kind = TIMELINE;
 
	var frames = layer.frames;
	if(layer.frameCount==0)return null;
 
	var nextFrame = 0;
	while(nextFrame<layer.frameCount)
	{
		var k = null;
	 
		var frame = frames[nextFrame];
		nextFrame += frame.duration;
		var n = frame.duration;
		
		if(frame.actionScript.search("stop")!=-1)
		{
			n = 0;
		}
	 
		if(frame.isEmpty)
		{
			k = new keyframe();
			k.next = n;
		}
		else
		{
			if(frame.tweenType=="none")
			{
				k = new keyframe();
				k.next = n;
				parseFromFrame(frame, k);
			}
			else if(frame.tweenType=="motion")
			{
				k = new tweenframe();
				k.next = n;
				parseFromFrame(frame, k);
			 
				k.easing = frame.tweenEasing;
			 
				// イージング先は次のフレームの最初のエレメント
				if(nextFrame<layer.frameCount)
				{
					var next = frames[nextFrame];
					if(next.elements.length>0)
					{
						var e = next.elements[0];
						parseFromElementToDraw(e, k.draw_after);
					}
				}
			}
		}
	 
		if(k!=null)
		{
			tl.child.push(k);
		}
	}
 
	return tl;
}

var draw_bases = new Array();
for(d in states)
{
	var st = states[d];
	// 有効なlayer数を確認
	var arLayer = new Array(); // 有効なlayerのindexを格納する
 
	if(lib.editItem(st.name))
	{
		fl.trace(st.name);
		var layers = doc.getTimeline().layers;
		for(l in layers)
		{
			//fl.trace(layers[l].name);
			if(layers[l].name.search("^col_")==-1
			&& layers[l].name.search("^none")==-1)
			{
				arLayer.push(l);
			}
		}
	 
		if(arLayer.length==0) continue;
		if(arLayer.length==1)
		{
			var j = arLayer[0];
			var t = parseFromLayer(layers[j]);
			t.id = getDrawBaseID(st.name);
			if(t!=null) draw_bases.push(t);
		}
		else
		{
			// layerはFlash上での上から取得できるので、書き出す時は逆順に辿る
			arLayer.reverse();
		 
			var movie = new movieclip();
			movie.base.id = getDrawBaseID(st.name);
			for(s in arLayer)
			{
				var t = parseFromLayer(layers[arLayer[s]]);
				if(t!=null) movie.base.child.push(t);
			}
			draw_bases.push(movie);
		}
	}
}

////////////////////////
// 最後にXMLに出力する
// XML定義
FLfile.write(xmlPath, "<?xml version=\"1.0\" encoding=\"Shift-JIS\"?>\r\n<draw_base_def>\r\n\r\n");
// texture
FLfile.write(xmlPath, "\t<texture id=\"" + sseName + "\" src=\"" + sseName + ".png\" />\r\n\r\n", "append");

// sprite 
for(i in sprite_bases)
{
	var sp = sprite_bases[i];
 
	var sp_str = "\t<sprite id=\"" + sp.base.id + "\">\r\n"
				+ "\t\t<pivot x=\"" + Math.round(sp.base.pivot.x) + "\" y=\"" + Math.round(sp.base.pivot.y) + "\" />\r\n"
				+ "\t\t<tex id=\"" + sseName + "\" />\r\n"
				+ "\t\t<rect left=\"" + sp.rect.left + "\" top=\"" + sp.rect.top + "\" right=\"" + sp.rect.right + "\" bottom=\"" + sp.rect.bottom + "\" />\r\n"
				+ "\t</sprite>"
				;
 
	FLfile.write(xmlPath, sp_str+"\r\n\r\n", "append");
}

FLfile.write(xmlPath, "\r\n", "append");

// draw_base
function strFromDraw(draw)
{
	var str = "<draw";

	if(draw.x!=0) str += " x=\"" + draw.x + "\"";
	if(draw.y!=0) str += " y=\"" + draw.y + "\"";
 
	if(draw.width!=1)	str += " width=\"" + draw.width + "\"";
	if(draw.height!=1)	str += " height=\"" + draw.height + "\""
	
	if(draw.angle!=0)	str += " angle=\"" + draw.angle + "\"";

	if(draw.a!=255 || draw.r!=255 || draw.g!=255 || draw.b!=255)
		str += " color=\"" + draw.a.toString(16) + draw.r.toString(16) + draw.g.toString(16) + draw.b.toString(16) + "\"";

	if(str=="<draw") return "";
	return str  + " />\r\n";
}

function strFromKeyFrame(key,tab)
{
	var frame="";
	if(key.base.kind==KEYFRAME)
	{
		frame="keyframe";
	}
	else if(key.base.kind==TWEEN)
	{
		frame="tween";
	}
 
	if(frame=="") return "";
 
	var k_str = tab + "\t<" + frame + ">\r\n"
			  + tab + "\t\t<next frame=\"" + key.next + "\" />\r\n"
		      ;
 
	if(key.base.kind==TWEEN)
	{
		// 最初のdrawは、childの一つ目
		var e = key.base.child[0];
		var t_draw = strFromDraw(key.base.child[0].draw);
		if(t_draw!="") k_str += tab + "\t\t" + t_draw;
	 
		t_draw= strFromDraw(key.draw_after)
		if(t_draw!="") k_str += tab + "\t\t" + t_draw;
	}
 
	// child
	var childs = key.base.child; 
	if(childs.length>0)
	{
		k_str += tab + "\t\t<child>\r\n";
		for(c in childs)
		{
			k_str += tab + "\t\t\t<base id=\"" + childs[c].id + "\"";
			if(c==0 && key.base.kind==TWEEN)
			{// TWEENの時は、0番はidのみでおわり
				k_str += " />\r\n";
			}
			else
			{
				var d_str = strFromDraw(childs[c].draw);
				if(d_str=="")				 
				{
					k_str += " />\r\n";
				}
				else
				{
					k_str += ">\r\n";
					k_str += tab + "\t\t\t\t" + d_str;
					k_str += tab + "\t\t\t</base>\r\n";
				}
			}
		}
		k_str += tab + "\t\t</child>\r\n";
	}
	return k_str + tab +"\t</" + frame + ">\r\n"
}

function strFromTimeline(tl, layer)
{
	var t_str = ""
	var frames = tl.child;
	if(frames.length>0)
	{
		if(layer)
			t_str = "\t\t<layer>\r\n";
		else
			t_str = "\t<timeline id=\"" + tl.id + "\">\r\n";
	
		for(f in frames)
		{
			if(layer)
				t_str += strFromKeyFrame(frames[f], "\t\t");
			else
				t_str += strFromKeyFrame(frames[f], "\t");
		}
	
		if(layer)
			t_str += "\t\t</layer>\r\n";
		else
			t_str += "\t</timeline>\r\n\r\n";
	}
 
	return t_str;
}

for(i in draw_bases)
{
	var base_str = "";
 
	var base = draw_bases[i];
	if(base.kind==TIMELINE)
	{
		base_str = strFromTimeline(base, false);
	}
	else if("base" in base)
	{
		if(base.base.kind==MOVIE_CLIP)
		{
			base_str = "\t<movieclip id=\"" + base.base.id + "\">\r\n"; 
			var layers = base.base.child;
			for(l in layers)
			{
				base_str += strFromTimeline(layers[l], true);
			}
			base_str += "\t</movieclip>\r\n\r\n";
	}
	}
 
	FLfile.write(xmlPath, base_str, "append");
}

FLfile.write(xmlPath, "</draw_base_def>\r\n","append");
