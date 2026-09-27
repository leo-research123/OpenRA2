// Building control: supplied 0x0045FA90, 0x0043DA80 and 0x00458810.
// Sparse spans follow the fixed, already-vendored EA Mission Editor/XCC
// vxl_file.cpp (Olaf van der Spek; see third-party notices). Raster traversal
// and lighting are calibrated to 0x00756590 / 0x00757980 in the supplied EXE.
// Existing VoxLib/MotLib own resources; the host owns only derived surfaces.
#include "building_voxel.hpp"
#include "techno_drawing.hpp"
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/Memory.h"
#include "yrpp/YRMath.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <string>
#include <limits>
#include <cctype>
#include <atomic>
namespace game {
namespace {
std::atomic<std::uint64_t> resource_revision{1};
constexpr double pi=3.14159265358979323846;
// The target runs these routines with x87 control word 0x0E7F. Stores to
// single precision truncate; host float expressions normally round nearest.
float store_float(double value){const float nearest=static_cast<float>(value);return std::abs(double(nearest))>std::abs(value)?std::bit_cast<float>(std::bit_cast<std::uint32_t>(nearest)-1u):nearest;}
double add_truncated(double a,double b){
 // TwoSum recovers the discarded residual without changing the host/thread
 // rounding mode. Even a cardinal-angle residual can change a later float.
 const double sum=a+b,virtual_b=sum-a;
 const double error=(a-(sum-virtual_b))+(b-virtual_b);
 return (sum>0&&error<0)||(sum<0&&error>0)?std::nextafter(sum,0.0):sum;
}
// 0x004CACB0 / 0x004CAD00, including negative odd-index rounding.
void original_sincos(float angle,float& sine,float& cosine){
 sine=static_cast<float>(Math::sin(angle));cosine=static_cast<float>(Math::cos(angle));
}
Matrix3D identity(){Matrix3D m;m.MakeIdentity();return m;}
Matrix3D rotation(int axis,float a){auto m=identity();float c,s;original_sincos(a,s,c);const int x=(axis+1)%3,y=(axis+2)%3;m.row[x][x]=c;m.row[x][y]=-s;m.row[y][x]=s;m.row[y][y]=c;return m;}
Matrix3D multiply(const Matrix3D&a,const Matrix3D&b){Matrix3D out;for(int y=0;y<3;++y)for(int x=0;x<4;++x){double value=add_truncated(double(a.row[y][2])*b.row[2][x],double(a.row[y][1])*b.row[1][x]);value=add_truncated(value,double(a.row[y][0])*b.row[0][x]);out.row[y][x]=store_float(x==3?add_truncated(value,a.row[y][3]):value);}return out;}
Vector3D<float> transform(const Matrix3D&m,Vector3D<float>p){std::array<float,3>out;for(int y=0;y<3;++y)out[y]=store_float(double(m.row[y][0])*p.X+double(m.row[y][1])*p.Y+double(m.row[y][2])*p.Z+m.row[y][3]);return {out[0],out[1],out[2]};}
unsigned direction(unsigned raw){return ((raw>>10)+1u)/2u&31u;}
float yaw(unsigned raw){return store_float((8-int(direction(raw)))*(pi/16.0));}
void translate_axis(Matrix3D&m,unsigned axis,float value){for(unsigned row=0;row<3;++row)m.row[row][3]=store_float(add_truncated(double(value)*m.row[row][axis],m.row[row][3]));}
void translate_vector(Matrix3D&m,Vector3D<float>p){
 // 0x005AE8F0 retains the Z-row accumulator in x87 across all three axes.
 const double z=add_truncated(add_truncated(add_truncated(double(p.X)*m.row[2][0],m.row[2][3]),double(p.Y)*m.row[2][1]),double(p.Z)*m.row[2][2]);
 translate_axis(m,0,p.X);translate_axis(m,1,p.Y);translate_axis(m,2,p.Z);m.row[2][3]=store_float(z);
}
bool load_pair(const std::string& name,VoxelStruct& pair){
 if(name.empty()||name=="<none>"||name=="none")return false;
 CCFileClass vf((name+".VXL").c_str());if(!vf.Exists())return false;
 auto* vox=GameCreate<VoxLib>(&vf,false);MotLib* hva=nullptr;
 try{CCFileClass hf((name+".HVA").c_str());hva=GameCreate<MotLib>(&hf);
  if(vox&&!vox->Initialized&&vox->CountHeaders&&hva&&!hva->LoadedFailed)
   hva->Scale(vox->leaSectionTailer(0,0)->HVAMultiplier);
 }catch(...){GameDelete(vox);GameDelete(hva);throw;}
 GameDelete(pair.VXL);GameDelete(pair.HVA);pair={vox,hva};
 resource_revision.fetch_add(1,std::memory_order_relaxed);
 return vox&&!vox->Initialized&&hva&&!hva->LoadedFailed;
}
bool ready(const VoxelStruct& v){return v.VXL&&!v.VXL->Initialized&&v.VXL->CountHeaders&&v.VXL->BodyData&&
 v.HVA&&!v.HVA->LoadedFailed&&v.HVA->Matrixes&&v.HVA->FrameCount>0&&v.HVA->LayerCount>=int(v.VXL->CountHeaders);}
bool finite(const Matrix3D&m){for(auto&r:m.row)for(unsigned i=0;i<4;++i)if(!std::isfinite(r[i])||std::abs(r[i])>100000)return false;return true;}
DrawingStatus copy_surface(const std::vector<std::uint32_t>&pixels,Vector3D<float>lo,Vector3D<float>hi,float cx,float cy,VoxelSurface&out){
 // 0x754649..0x75470F retains four pixels of padding around the projected
 // bounds. PlainBlit's row-depth phase depends on this full rectangle.
 const int width=int(double(hi.X)-lo.X)+8,height=int(double(hi.Y)-lo.Y)+8;
 const int left=128-width/2,top=128-height/2;
 if(width<=0||height<=0||left<0||top<0||left+width>256||top+height>256)return DrawingStatus::unsupported;
 if(std::none_of(pixels.begin(),pixels.end(),[](auto pixel){return pixel!=0;}))return DrawingStatus::skipped;
 out.width=width;out.height=height;out.offset={int(cx)-width/2,int(cy)-height/2};out.pixels.resize(std::size_t(width)*height);
 for(int y=0;y<height;++y)std::copy_n(pixels.data()+(top+y)*256+left,width,out.pixels.data()+y*width);
 return DrawingStatus::drawn;
}
struct Sample {unsigned x,y,z;std::uint8_t color,normal;};
bool decode(const VoxLib& v,const VoxelSectionTailer& t,std::vector<Sample>& out){
 const unsigned nx=std::uint8_t(t.size_X),ny=std::uint8_t(t.size_Y),nz=std::uint8_t(t.size_Z);
 if(!nx||!ny||!nz)return false;
 const auto begin=reinterpret_cast<std::uintptr_t>(v.BodyData);
 const auto end=begin+v.TotalSize;
 auto range=[&](const void*p,std::size_t n){auto q=reinterpret_cast<std::uintptr_t>(p);return q>=begin&&q<=end&&n<=end-q;};
 if(!range(t.span_start_off,std::size_t(nx)*ny*4)||!range(t.span_end_off,std::size_t(nx)*ny*4)||!range(t.span_data_off,0))return false;
 for(unsigned y=0;y<ny;++y)for(unsigned x=0;x<nx;++x){
  std::int32_t first,last;std::memcpy(&first,reinterpret_cast<const char*>(t.span_start_off)+4*(y*nx+x),4);std::memcpy(&last,reinterpret_cast<const char*>(t.span_end_off)+4*(y*nx+x),4);
  if(first==-1&&last==-1)continue;if(first<0||last<first)return false;
  const auto base=reinterpret_cast<std::uintptr_t>(t.span_data_off);if(std::uint64_t(last)>=end-base)return false;
  const auto*p=t.span_data_off+first;const auto*limit=t.span_data_off+last+1;unsigned z=0;
  while(z<nz){if(limit-p<3)return false;const unsigned skip=*p++,count=*p++;if((!skip&&!count)||skip>nz-z||count>nz-z-skip||limit-p<std::ptrdiff_t(2*count+1))return false;z+=skip;
   for(unsigned k=0;k<count;++k){out.push_back({x,y,z++,p[0],p[1]});p+=2;if(out.size()>16777216)return false;}
   if(*p++!=count)return false;
  }
  if(p!=limit)return false;
 }
 return true;
}
// Normals are exact float constants extracted from the supplied executable.
#include "building_voxel_normals.inc"
Vector3D<float> normalized(Vector3D<float>v){double len=std::sqrt(double(v.X)*v.X+double(v.Y)*v.Y+double(v.Z)*v.Z);if(len>0){v.X=store_float(v.X/len);v.Y=store_float(v.Y/len);v.Z=store_float(v.Z/len);}return v;}
double dot(Vector3D<float>a,Vector3D<float>b){return double(a.Z)*b.Z+double(a.Y)*b.Y+double(a.X)*b.X;}
Vector3D<float> voxel_light_direction(){
 const float xy=std::bit_cast<float>(std::uint32_t(0xbf3504e6));
 return transform(rotation(1,0.78537499904632568359375f),{xy,xy,0});
}
// 0x00753D00 calls orthogonal inverse 0x005AFC20 before 0x007586F0.
// That inverse transposes the basis even in the mixed scaled-barrel case.
// HVA is deliberately absent: the original builds this table BEFORE the
// per-limb/per-frame transforms. These are target constants, not a new light.
std::array<std::uint8_t,256> make_lighting(unsigned mode,const Matrix3D& scene) {
 std::array<std::uint8_t,256> table{};
 const auto world=voxel_light_direction();
 Vector3D<float> light{store_float(double(scene.row[0][0])*world.X+double(scene.row[1][0])*world.Y+double(scene.row[2][0])*world.Z),
  store_float(double(scene.row[0][1])*world.X+double(scene.row[1][1])*world.Y+double(scene.row[2][1])*world.Z),
  store_float(double(scene.row[0][2])*world.X+double(scene.row[1][2])*world.Y+double(scene.row[2][2])*world.Z)};
 const auto half=normalized(Vector3D<float>{light.X,light.Y,store_float(double(light.Z)+1)});
 if(mode>0&&mode<=4)for(unsigned i=0;i<normal_counts[mode];++i){
  const auto n=normal_tables[mode][i];
  const double diffuse=std::max(0.0,dot(n,light)),h=dot(n,half);
  const float specular=store_float(std::max(0.0,h/(3.0-2.0*h)));
  table[i]=std::uint8_t(std::clamp(int((diffuse+specular)*16),0,255));
 }
 table[253]=table[254]=table[255]=16;
 return table;
}
std::uint8_t lit_color(const Sample&s,const std::array<std::uint8_t,256>&table,const VoxelPalette&p){
 if(!s.color)return 0;
 return p.lighting[std::min(unsigned(table[s.normal]),p.sections-1)*256+s.color];
}
}
bool load_building_voxels(BuildingTypeClass&t) noexcept {try{
 if(!t.TurretAnimIsVoxel&&!t.BarrelAnimIsVoxel)return true;
 std::string name=t.BuildingAnim[9].Anim;for(auto&c:name)c=char(std::toupper(static_cast<unsigned char>(c)));
 // The target skips the first four characters when looking for TUR.
 const auto tur=name.size()>4?name.find("TUR",4):std::string::npos;
 if(tur!=std::string::npos){load_pair(name,t.TurretVoxel);name.replace(tur,std::string::npos,"BARL");}
 else if(t.BarrelAnimIsVoxel)name=t.VoxelBarrelFile;
 load_pair(name,t.BarrelVoxel);return true;
 }catch(...){return false;}}
bool load_object_voxels(ObjectTypeClass& type) noexcept {try{
 if(!type.Voxel)return true;
 std::string name=type.ImageFile;
 if(!load_pair(name,type.MainVoxel)){
  // 0x5F8272 / 0x5F8A6A..0x5F8B13: failed main resources clear the
  // owned main/turret pair, including a previous successful load. The
  // Bullet INI caller at 0x46C417 still returns success at 0x46C41C.
  const auto release=[](VoxelStruct& pair){GameDelete(pair.VXL);GameDelete(pair.HVA);pair={};};
  release(type.MainVoxel);
  if(type.WhatAmI()==AbstractType::UnitType&&static_cast<TechnoTypeClass&>(type).HasMultipleTurrets()&&
     !static_cast<TechnoTypeClass&>(type).IsGattling){
   // 0x5F8080 releases all 18 charger turret/barrel pairs on this branch.
   for(auto& pair:type.ChargerTurrets)release(pair);
   for(auto& pair:type.ChargerBarrels)release(pair);
  }else release(type.TurretVoxel);
  resource_revision.fetch_add(1,std::memory_order_relaxed);
  return true;
 }
 if((type.WhatAmI()==AbstractType::UnitType||type.WhatAmI()==AbstractType::AircraftType)&&static_cast<TechnoTypeClass&>(type).Turret){
  auto&techno=static_cast<TechnoTypeClass&>(type);
  if(type.WhatAmI()==AbstractType::UnitType&&techno.HasMultipleTurrets()&&!techno.IsGattling){
   // ObjectType.LoadVoxel 0x5F8110 -> 0x5F7A90 / 0x5F7DB0:
   // slot zero has no numeric suffix; missing barrel files are allowed.
   if(techno.TurretCount>18)return false;
   for(int i=0;i<techno.TurretCount;++i){const auto suffix=i?std::to_string(i):std::string{};
    load_pair(name+"TUR"+suffix,type.ChargerTurrets[i]);
    if(!ready(type.ChargerTurrets[i]))return false;
    load_pair(name+"BARL"+suffix,type.ChargerBarrels[i]);
   }
  }else{load_pair(name+"TUR",type.TurretVoxel);load_pair(name+"BARL",type.BarrelVoxel);}
 }
 return true;
 }catch(...){return false;}}
namespace {
struct VoxelPartsSink {BuildingVoxelPart* parts;unsigned capacity,count=0;ObjectTypeClass* type;};
void bind_voxel_collection(TechnoDrawing& drawing,VoxelPartsSink& sink){
 drawing.context=&sink;
 drawing.palette=[](void*,const TechnoClass&,TechnoPalette,CellClass*,HouseClass*,const DrawingPaletteHandle*& output) noexcept {
  output=nullptr;return DrawingStatus::drawn;
 };
 drawing.voxel=[](void* p,const TechnoVoxelRequest& r) noexcept {
  auto& sink=*static_cast<VoxelPartsSink*>(p);if(sink.count>=sink.capacity)return DrawingStatus::invalid_argument;
  auto& part=sink.parts[sink.count++];part.resource=r.resource;part.local=r.local_matrix?*r.local_matrix:r.matrix;
  part.frame=unsigned(r.frame);part.use_buffer=r.use_buffer;part.shadow=r.shadow;part.cache_key=r.cache_key;
  part.shadow_layer=r.shadow_layer;part.half_shadow=r.half_shadow;part.camera_applied=!r.local_matrix;
  part.barrel=r.resource==&sink.type->BarrelVoxel;
  for(const auto& barrel:sink.type->ChargerBarrels)if(r.resource==&barrel)part.barrel=true;
  return DrawingStatus::drawn;
 };
}
}
// Legacy internal inspection adapters. Algorithms execute the same original
// methods used by the map; these functions only collect their borrowed output.
bool unit_voxel_parts(UnitClass& unit,BuildingVoxelPart* parts,unsigned capacity,unsigned& count) noexcept {
 count=0;if(!parts||capacity<4||!unit.Type||!unit.Locomotor)return false;
 auto* original=unit.Type;
 if(unit.Unloading&&original->Harvester&&original->UnloadingClass)unit.Type=original->UnloadingClass;
 VoxelPartsSink sink{parts,capacity,0,unit.Type};TechnoDrawing drawing;bind_voxel_collection(drawing,sink);
 const auto status=with_techno_drawing(drawing,[](void* p){static_cast<UnitClass*>(p)->DrawAsVXL({}, {},1000,0);},&unit);
 unit.Type=original;count=sink.count;return techno_drawing_complete(status)&&count!=0;
}
bool load_voxel_palette(VoxelPalette&out) noexcept {try{
 out={};CCFileClass file("VOXELS.VPL");if(!file.Open(FileAccessMode::Read))return false;
 struct Header{std::uint32_t first,last,count,reserved;} h{};std::uint8_t pal[768];
 bool ok=file.ReadBytes(&h,sizeof(h))==sizeof(h)&&h.first<=h.last&&h.last<256&&h.count>0&&h.count<=128&&file.ReadBytes(pal,sizeof(pal))==sizeof(pal);
 if(ok){out.lighting.resize(h.count*256);ok=file.ReadBytes(out.lighting.data(),int(out.lighting.size()))==int(out.lighting.size());}file.Close();
 if(!ok){out={};return false;}out.sections=h.count;return true;
 }catch(...){out={};return false;}}
Matrix3D building_voxel_camera() noexcept{return multiply(rotation(0,store_float(-pi/3)),rotation(2,store_float(-pi/4)));}
bool building_voxel_plan(const BuildingClass& building,BuildingVoxelPlan& output) noexcept {
 output.count=0;output.mixed=false;output.barrel_after_shp=false;if(!building.Type)return false;
 VoxelPartsSink sink{output.parts.data(),unsigned(output.parts.size()),0,building.Type};
 TechnoDrawing drawing;bind_voxel_collection(drawing,sink);
 const auto status=with_techno_drawing(drawing,[](void* p){static_cast<BuildingClass*>(p)->Draw({},{});},const_cast<BuildingClass*>(&building));
 output.count=sink.count;
 // These are presentation-inspection labels, not an independent draw path.
 output.mixed=building.Type->BarrelAnimIsVoxel&&!building.Type->TurretAnimIsVoxel;
 output.barrel_after_shp=output.mixed&&((direction(building.PrimaryFacing.Current().Raw)+28)%32)<=16;
 return techno_drawing_complete(status)&&output.count!=0;
}
DrawingStatus render_building_voxel(const BuildingVoxelPart&part,const VoxelPalette&palette,VoxelSurface&out) noexcept {try{
 out={};if(!part.resource||!ready(*part.resource)||!palette.sections||palette.lighting.size()!=palette.sections*256)return DrawingStatus::unavailable;
 const auto&v=*part.resource->VXL;const auto&h=*part.resource->HVA;const auto camera=part.camera_applied?identity():building_voxel_camera();
 if(part.shadow){
  // 0x753F90 / 0x756860 project occupied XY columns from the section's
  // bottom plane. Shadow spans always cover the sample and its right
  // neighbour; flattening the full model before the camera is different.
  const int layer=part.shadow_layer;
  if(layer<0||unsigned(layer)>=v.CountHeaders)return DrawingStatus::invalid_argument;
  const int limb=v.HeaderData[layer].limb_number;
  if(limb<0||unsigned(limb)>=v.CountTailers)return DrawingStatus::invalid_argument;
  const auto&t=v.TailerData[limb];const unsigned nx=std::uint8_t(t.size_X),ny=std::uint8_t(t.size_Y);
  if(!nx||!ny)return DrawingStatus::invalid_argument;
  const auto matrix=multiply(multiply(camera,part.local),h.GetLayerMatrix(layer,0));
  if(!finite(matrix))return DrawingStatus::invalid_argument;
  Vector3D<float> points[4],lo{1e9f,1e9f,0},hi{-1e9f,-1e9f,0};
  // 0x754C00 derives the shadow displacement from the same light vector.
  const float displacement=store_float(double(voxel_light_direction().X)*-6);
  for(unsigned i=0;i<4;++i){auto p=transform(matrix,t.Bounds[i]);if(part.half_shadow){p.X=store_float(double(p.X)*0.5);p.Y=store_float(double(p.Y)*0.5);}p.X=store_float(double(p.X)+displacement);p.Y=-p.Y;points[i]=p;lo.X=std::min(lo.X,p.X);lo.Y=std::min(lo.Y,p.Y);hi.X=std::max(hi.X,p.X);hi.Y=std::max(hi.Y,p.Y);}
  if(hi.X-lo.X>250||hi.Y-lo.Y>250)return DrawingStatus::unsupported;
  const float cx=store_float((double(lo.X)+hi.X)*0.5),cy=store_float((double(lo.Y)+hi.Y)*0.5);
  const int sx=int((double(points[2].X)+128-cx)*256),sy=int((double(points[2].Y)+128-cy)*256);
  const int xx=int((double(points[1].X)-points[2].X)/nx*256),xy=int((double(points[1].Y)-points[2].Y)/nx*256);
  const int yx=int((double(points[3].X)-points[2].X)/ny*256),yy=int((double(points[3].Y)-points[2].Y)/ny*256);
  std::vector<Sample> samples;if(!decode(v,t,samples))return DrawingStatus::invalid_argument;
  std::vector<std::uint32_t> pixels(256*256);
  for(unsigned y=0;y<ny;++y)for(unsigned x=0;x<nx;++x){
   std::int32_t first;std::memcpy(&first,reinterpret_cast<const char*>(t.span_start_off)+4*(y*nx+x),4);if(first==-1)continue;
   const unsigned px=std::uint16_t(sx+int(x)*xx+int(y)*yx)>>8,py=std::uint16_t(sy+int(x)*xy+int(y)*yy)>>8;
   pixels[py*256+px]=0x1000001u;if(px<255)pixels[py*256+px+1]=0x1000001u;
  }
  return copy_surface(pixels,lo,hi,cx,cy,out);
 }

 struct Section {const VoxelSectionTailer*t;Matrix3D local;std::array<Vector3D<float>,8> points;std::vector<Sample> samples;unsigned corner=0;};
 std::vector<Section> sections;Vector3D<float>lo{1e9f,1e9f,1e9f},hi{-1e9f,-1e9f,-1e9f};
 for(unsigned i=0;i<v.CountHeaders;++i){const int limb=v.HeaderData[i].limb_number;if(limb<0||unsigned(limb)>=v.CountTailers)return DrawingStatus::invalid_argument;Section s;s.t=&v.TailerData[limb];s.local=part.local*h.GetLayerMatrix(i,part.frame);if(!finite(s.local))return DrawingStatus::invalid_argument;
  const auto matrix=multiply(multiply(camera,part.local),h.GetLayerMatrix(i,part.frame));for(unsigned k=0;k<8;++k){auto p=transform(matrix,s.t->Bounds[k]);p.Y=-p.Y;s.points[k]=p;if(k&&p.Z<s.points[s.corner].Z)s.corner=k;
   if(!std::isfinite(p.X)||!std::isfinite(p.Y)||!std::isfinite(p.Z))return DrawingStatus::invalid_argument;
   lo.X=std::min(lo.X,p.X);lo.Y=std::min(lo.Y,p.Y);lo.Z=std::min(lo.Z,p.Z);hi.X=std::max(hi.X,p.X);hi.Y=std::max(hi.Y,p.Y);hi.Z=std::max(hi.Z,p.Z);}
  if(!decode(v,*s.t,s.samples))return DrawingStatus::invalid_argument;sections.push_back(std::move(s));
 }
 const auto lighting=make_lighting(std::uint8_t(sections.front().t->NormalsMode),multiply(camera,part.local));
 if(hi.X-lo.X>250||hi.Y-lo.Y>250||hi.Z-lo.Z>250)return DrawingStatus::unsupported;
 const Vector3D<float>center{store_float((double(lo.X)+hi.X)*0.5),store_float((double(lo.Y)+hi.Y)*0.5),store_float((double(lo.Z)+hi.Z)*0.5)};
 std::vector<std::uint32_t> pixels(256*256);std::vector<unsigned char> depth(256*256);
 constexpr int corners[8][8]={{0,3,1,4,1,1,-1,-1},{1,2,0,5,1,0,-1,1},{2,1,3,6,0,0,1,1},{3,0,2,7,0,1,1,-1},{4,7,5,0,1,1,-1,-1},{5,6,4,1,1,0,-1,1},{6,5,7,2,0,0,1,1},{7,4,6,3,0,1,1,-1}};
 auto maximum_z=[](const auto&s){float z=s.points[0].Z;for(const auto&p:s.points)z=std::max(z,p.Z);return z;};
 std::stable_sort(sections.begin(),sections.end(),[&](const auto&a,const auto&b){return maximum_z(a)<maximum_z(b);});
 if(part.use_buffer)std::reverse(sections.begin(),sections.end());
 for(auto&s:sections){const auto*c=corners[s.corner];const auto origin=s.points[c[0]];int start[3],step[3][3];const unsigned sizes[3]={std::uint8_t(s.t->size_X),std::uint8_t(s.t->size_Y),std::uint8_t(s.t->size_Z)};
  for(unsigned axis=0;axis<3;++axis){const float o=axis==0?origin.X:axis==1?origin.Y:origin.Z,mid=axis==0?center.X:axis==1?center.Y:center.Z;start[axis]=int((double(o)+128-mid)*256);for(unsigned d=0;d<3;++d){const auto&q=s.points[c[d+1]];float end=axis==0?q.X:axis==1?q.Y:q.Z;step[d][axis]=int((double(end)-o)/sizes[d]*256);}}
  std::stable_sort(s.samples.begin(),s.samples.end(),[&](const auto&a,const auto&b){if(a.y!=b.y)return c[7]>0?a.y<b.y:a.y>b.y;if(a.x!=b.x)return c[6]>0?a.x<b.x:a.x>b.x;return s.corner<4?a.z<b.z:a.z>b.z;});
  auto packed_xy=[](int x,int y){return std::uint32_t(std::uint16_t(x))|(std::uint32_t(std::uint16_t(y))<<16);};
  unsigned last_x=UINT32_MAX,last_y=UINT32_MAX,cursor_z=0;std::uint32_t xy=0;
  for(const auto&sample:s.samples){unsigned ijk[3]={c[4]?sizes[0]-1-sample.x:sample.x,c[5]?sizes[1]-1-sample.y:sample.y,s.corner>=4?sizes[2]-1-sample.z:sample.z};unsigned p[3];for(unsigned a=0;a<3;++a)p[a]=std::uint16_t(start[a]+int(ijk[0])*step[0][a]+int(ijk[1])*step[1][a]+int(ijk[2])*step[2][a])>>8;
   if(!part.use_buffer){
    // 0x007DF9C0 / 0x007DFAE0 add packed X/Y words: X carries into Y.
    // A skipped run uses a separately accumulated pair, unlike dense voxels.
    if(last_x!=sample.x||last_y!=sample.y){xy=packed_xy(start[0],start[1])+ijk[0]*packed_xy(step[0][0],step[0][1])+ijk[1]*packed_xy(step[1][0],step[1][1]);cursor_z=0;last_x=sample.x;last_y=sample.y;}
    const int skip=int(ijk[2]-cursor_z);xy+=packed_xy(skip*step[2][0],skip*step[2][1]);
    p[0]=(xy>>8)&255;p[1]=xy>>24;xy+=packed_xy(step[2][0],step[2][1]);cursor_z=ijk[2]+1;
   }
   const auto color=lit_color(sample,lighting,palette);const auto index=p[1]*256+p[0];if(part.use_buffer&&p[2]<=depth[index])continue;
   // Original depth-enabled span routine writes the sample and its right-hand
   // neighbor together after testing the first pixel (0x00757279..0x00757294).
   const auto delta=std::int16_t(-int(p[2])+128-int(center.Z));const auto packed=0x1000000u|(std::uint32_t(std::uint16_t(delta))<<8)|color;
   pixels[index]=packed;depth[index]=p[2];if(part.use_buffer&&p[0]<255){pixels[index+1]=packed;depth[index+1]=p[2];}
  }
 }
 return copy_surface(pixels,lo,hi,center.X,center.Y,out);
 }catch(...){out={};return DrawingStatus::backend_failure;}}
std::uint64_t voxel_resource_revision() noexcept {return resource_revision.load(std::memory_order_relaxed);}
std::uint64_t building_voxel_signature(const BuildingClass&b) noexcept {
 std::uint64_t h=1469598103934665603ull;auto add=[&](std::uint64_t v){h^=v;h*=1099511628211ull;};add(voxel_resource_revision());add(b.PrimaryFacing.Current().Raw);add(b.BarrelFacing.Current().Raw);add(b.TurretAnimFrame);add(unsigned(b.CurrentMission));add(unsigned(b.QueuedMission));add(b.Animation.Value);add(b.Health);add(b.Type&&b.Type->UseBuffer);add(std::bit_cast<std::uint32_t>(b.TurretRecoil.TravelSoFar));add(std::bit_cast<std::uint32_t>(b.BarrelRecoil.TravelSoFar));add(unsigned(b.TurretRecoil.State));add(unsigned(b.BarrelRecoil.State));return h;
}
}
