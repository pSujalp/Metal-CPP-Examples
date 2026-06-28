#include "MyMTKViewDelegate.hpp"
#include "Time.hpp"


MyMTKViewDelegate::MyMTKViewDelegate( MTL::Device* pDevice ): MTK::ViewDelegate() , _pRenderer( new Renderer( pDevice ) ){}

MyMTKViewDelegate::~MyMTKViewDelegate()

{
    delete _pRenderer;
}

void MyMTKViewDelegate::drawInMTKView( MTK::View* pView )
{
    Time::Update();
    _pRenderer->draw( pView );
}
