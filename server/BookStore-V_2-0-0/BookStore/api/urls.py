# URLs
from django.urls import path, include
from rest_framework.routers import DefaultRouter
from .views import *
from drf_yasg.views import get_schema_view
from drf_yasg import openapi
from rest_framework.permissions import AllowAny

router = DefaultRouter()
router.register('users', UserViewSet)
router.register('books', BookViewSet)
router.register('cart', CartItemViewSet)

schema_view = get_schema_view(
    openapi.Info(
        title="Bookstore API",
        default_version='v1',
        description="API documentation for the Bookstore project",
        terms_of_service="https://www.google.com/policies/terms/",
        contact=openapi.Contact(email="support@bookstore.com"),
        license=openapi.License(name="BSD License"),
    ),
    public=True,
    permission_classes=(AllowAny,),
)

urlpatterns = [
    path('api/', include(router.urls)),
    path('api/user-detail/', UserDetailView.as_view(), name='user-detail'),
    path('swagger/', schema_view.with_ui('swagger', cache_timeout=0), name='schema-swagger-ui'),
    path('redoc/', schema_view.with_ui('redoc', cache_timeout=0), name='schema-redoc'),
]